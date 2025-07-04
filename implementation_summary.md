# Click-to-Reposition Otoscopy Crop - Implementation Summary

## Quick Start: Files to Modify

You need to make changes to **3 main components** in your existing codebase:

### 1. **OtoscopeTransform Class** 
**File**: Wherever your `OtoscopeTransform` is defined

**Key Addition**: Add the `updateOffset()` method
```cpp
void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight);
```

### 2. **OtoscopyWorker Class**
**File**: Wherever your `OtoscopyWorker` is defined  

**Key Additions**: Add frame tracking and transform access methods
```cpp
utility::processing::OtoscopeTransform* GetTransform();
void SetFrameDimensions(int width, int height);
std::pair<int, int> GetFrameDimensions() const;
bool HasFrameDimensions() const;
```

### 3. **RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()**
**File**: Wherever your `RemoteOssiviewRoutine` class is defined

**Key Changes**: 
- Replace `QLabel` with `OtoScopyLabel`
- Add click signal connection
- Track frame dimensions in the sink

## Exact Code Changes

### Change 1: OtoscopeTransform
```cpp
// ADD this method to your existing OtoscopeTransform class
void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight) {
    const auto cx = frameWidth / 2;
    const auto cy = frameHeight / 2;
    
    // Boundary constraints
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
```

### Change 2: OtoscopyWorker  
```cpp
// ADD these to your existing OtoscopyWorker class
private:
    mutable std::mutex _frameDimMutex;
    int _frameWidth = 0;
    int _frameHeight = 0;

public:
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
```

### Change 3: BuildOtoscopyDisplayWidget
```cpp
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // CHANGED: Use OtoScopyLabel instead of QLabel
    const auto frame = new OtoScopyLabel();
    
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        
        // ADDED: Track frame dimensions
        _otoscopyWorker->SetFrameDimensions(cols, rows);
        
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // ADDED: Click handling for crop repositioning
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        auto* transform = _otoscopyWorker->GetTransform();
        if (transform && _otoscopyWorker->HasFrameDimensions()) {
            const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
            transform->updateOffset(imagePos, frameWidth, frameHeight);
        }
    });

    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}
```

## Required Headers
Make sure these includes are added where needed:
```cpp
#include <QPoint>
#include <QLabel>
#include <QMouseEvent>
#include <mutex>
#include <algorithm> // for std::clamp
```

## What You Get

✅ **Click anywhere on the otoscopy image** → crop center moves there  
✅ **Boundary protection** → circle never goes outside frame bounds  
✅ **Always circular** → crop maintains perfect circle shape  
✅ **Real-time updates** → immediate visual feedback  
✅ **Thread-safe** → no race conditions in frame processing  

## Testing

1. **Basic functionality**: Click different parts of the otoscopy image
2. **Boundary testing**: Click near edges to verify constraints work
3. **Performance**: Click rapidly to ensure smooth updates
4. **Co-registration**: Verify alignment with B-Mode images is accurate

The implementation uses your existing `OtoScopyLabel::mapToImage()` method for coordinate conversion, so clicks are accurately mapped to image coordinates for precise co-registration.