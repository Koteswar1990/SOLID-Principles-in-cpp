/*
 * INTEGRATION GUIDE FOR CLICK-TO-REPOSITION OTOSCOPY CROP
 * 
 * This file shows exactly how to modify your existing code to implement
 * the click-to-reposition functionality. Follow these steps in order.
 */

// =============================================================================
// STEP 1: Update your OtoscopeTransform class definition
// =============================================================================

// In your OtoscopeTransform header file, replace the existing class with:
namespace utility::processing {

class OtoscopeTransform {
private:
    std::pair<int, int> _offset;
    uint32_t _cropRadius;
    uint32_t _maskRadius;
    mutable std::optional<xt::xarray<bool>> _mask;
    std::function<size_t(size_t, size_t, size_t)> _xCorrection;
    std::function<size_t(size_t, size_t, size_t)> _yCorrection;

    void updateCorrectionFunctions() {
        if (_offset.first > 0) {
            _xCorrection = [](size_t, size_t value, size_t max) { return std::min(value, max); };
        } else {
            _xCorrection = [](size_t min, size_t value, size_t) { return std::max(min, value); };
        }

        if (_offset.second > 0) {
            _yCorrection = [](size_t, size_t value, size_t max) { return std::min(value, max); };
        } else {
            _yCorrection = [](size_t min, size_t value, size_t) { return std::max(min, value); };
        }
    }

public:
    OtoscopeTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius)
        : _offset(offset), _cropRadius(cropRadius), _maskRadius(maskRadius), _mask(std::nullopt)
    {
        updateCorrectionFunctions();
    }

    // ADD THIS NEW METHOD for dynamic offset updates
    void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight) {
        const auto cx = frameWidth / 2;
        const auto cy = frameHeight / 2;
        
        // Apply boundary constraints
        const auto minCenterX = static_cast<int>(_cropRadius);
        const auto maxCenterX = frameWidth - static_cast<int>(_cropRadius);
        const auto minCenterY = static_cast<int>(_cropRadius);
        const auto maxCenterY = frameHeight - static_cast<int>(_cropRadius);
        
        const auto clampedCenterX = std::clamp(newCenter.x(), minCenterX, maxCenterX);
        const auto clampedCenterY = std::clamp(newCenter.y(), minCenterY, maxCenterY);
        
        _offset.first = clampedCenterX - cx;
        _offset.second = clampedCenterY - cy;
        
        updateCorrectionFunctions();
        _mask = std::nullopt; // Force mask regeneration
    }
    
    // ADD THESE UTILITY METHODS
    std::pair<int, int> getCurrentOffset() const {
        return _offset;
    }
    
    std::pair<int, int> getCurrentCenter(int frameWidth, int frameHeight) const {
        const auto cx = frameWidth / 2;
        const auto cy = frameHeight / 2;
        return {cx + _offset.first, cy + _offset.second};
    }

    // Keep your existing operator() method unchanged
    types::VideoFrameDescriptor operator()(types::VideoFrameDescriptor frame) {
        const auto [cx, cy] = std::make_pair(frame.columns / 2, frame.rows / 2);
        const auto offsetX = _xCorrection(0, cx + _offset.first, frame.columns);
        const auto offsetY = _yCorrection(0, cy + _offset.second, frame.rows);
        const auto center = std::make_pair(offsetX, offsetY);
        auto trimmed = TrimImage(std::move(frame), center, _cropRadius, _cropRadius);
        
        if (!_mask.has_value()) {
            _mask = GenerateMask(trimmed.rows, trimmed.columns, _maskRadius);
        }
        
        auto maskedImage = utility::processing::MaskImage(_mask.value(), trimmed);
        return maskedImage;
    }
};

} // namespace utility::processing

// =============================================================================
// STEP 2: Update your OtoscopyWorker class
// =============================================================================

// In your OtoscopyWorker class, ADD these members and methods:
namespace imaging {

class OtoscopyWorker {
private:
    // Your existing members...
    utility::processing::OtoscopeTransform _transform;
    
    // ADD THESE NEW MEMBERS for frame tracking
    mutable std::mutex _frameDimMutex;
    int _frameWidth = 0;
    int _frameHeight = 0;

public:
    // Keep your existing constructor, just make sure it stores the transform
    
    // ADD THESE NEW METHODS
    utility::processing::OtoscopeTransform* GetTransform() {
        return &_transform;
    }
    
    void SetFrameDimensions(int width, int height) {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        _frameWidth = width;
        _frameHeight = height;
    }
    
    std::pair<int, int> GetFrameDimensions() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return {_frameWidth, _frameHeight};
    }
    
    bool HasFrameDimensions() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameWidth > 0 && _frameHeight > 0;
    }
    
    // Keep all your existing methods...
};

} // namespace imaging

// =============================================================================
// STEP 3: Update your RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget method
// =============================================================================

// In your RemoteOssiviewRoutine class, REPLACE the existing method with:
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // CHANGE: Use OtoScopyLabel instead of QLabel
    const auto frame = new OtoScopyLabel();
    
    // MODIFY: Enhanced sink that tracks frame dimensions
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        
        // ADD: Update frame dimensions in worker
        _otoscopyWorker->SetFrameDimensions(cols, rows);
        
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // ADD: Connect click signal to update crop center
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        auto* transform = _otoscopyWorker->GetTransform();
        if (transform && _otoscopyWorker->HasFrameDimensions()) {
            const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
            transform->updateOffset(imagePos, frameWidth, frameHeight);
            
            // Optional debug output
            qDebug() << "Crop center updated to:" << imagePos;
        }
    });

    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}

// =============================================================================
// STEP 4: Add required includes
// =============================================================================

// At the top of your files, make sure you have these includes:
/*
#include <QPoint>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPen>
#include <mutex>
#include <algorithm> // for std::clamp
*/

// =============================================================================
// STEP 5: Optional - Add visual feedback
// =============================================================================

// If you want visual feedback showing the crop center, you can use the 
// OtoScopyLabelWithCenterTracking class and update your click handler:

/*
connect(frame, &OtoScopyLabel::clickedAt, this, [this, frame](const QPoint& imagePos) {
    auto* transform = _otoscopyWorker->GetTransform();
    if (transform && _otoscopyWorker->HasFrameDimensions()) {
        const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
        transform->updateOffset(imagePos, frameWidth, frameHeight);
        
        // Update visual indicator if using the enhanced label
        if (auto* enhancedFrame = qobject_cast<OtoScopyLabelWithCenterTracking*>(frame)) {
            const auto newCenter = transform->getCurrentCenter(frameWidth, frameHeight);
            enhancedFrame->setCropCenter(QPoint(newCenter.first, newCenter.second), cropRadius);
        }
    }
});
*/

// =============================================================================
// SUMMARY OF CHANGES
// =============================================================================

/*
1. ✅ Add updateOffset() method to OtoscopeTransform
2. ✅ Add frame dimension tracking to OtoscopyWorker  
3. ✅ Replace QLabel with OtoScopyLabel in BuildOtoscopyDisplayWidget
4. ✅ Connect clickedAt signal to update crop center
5. ✅ Add boundary checking to keep circle within frame
6. ✅ Automatic mask regeneration when center changes

TESTING:
- Click anywhere on the otoscopy image
- The circular crop should move to center on that point
- The crop should never go outside the frame boundaries
- The shape should remain perfectly circular

INTEGRATION NOTES:
- No changes needed to TrimImage, GenerateMask, or MaskImage functions
- Your existing overlay and display pipeline remains unchanged
- The click handling works with the existing coordinate mapping
- Thread-safe frame dimension tracking prevents race conditions
*/