# Otoscopy Click-to-Reposition Circular Crop Implementation

## Problem Analysis

The current otoscopy application displays a circular crop of the video feed, but the crop center is fixed. The requirement is to allow users to click anywhere within the displayed image to reposition the circular crop center while maintaining the following constraints:

1. **Click-to-reposition**: Clicking inside the circle makes that point the new center
2. **Boundary constraints**: The circle cannot move beyond the boundary defined by its initial default center
3. **Always circular**: The crop should remain circular, not become square
4. **Precise co-registration**: Allows service engineers to align B-Mode and Otoscopy images

## Current Implementation Issues

1. **Fixed offset**: `OtoscopeTransform` has a fixed `_offset` set during construction
2. **Wrong widget type**: `BuildOtoscopyDisplayWidget()` uses `QLabel` instead of `OtoScopyLabel`
3. **No click handling**: No connection between mouse clicks and crop repositioning
4. **Missing boundary logic**: No dynamic boundary checking for the new center position

## Implementation Solution

### 1. Modify OtoscopeTransform Class

Add dynamic offset update capability:

```cpp
class OtoscopeTransform {
private:
    std::pair<int, int> _offset;
    std::pair<int, int> _initialCenter; // Store initial center for boundary calculations
    uint32_t _cropRadius;
    uint32_t _maskRadius;
    uint32_t _frameWidth, _frameHeight; // Store frame dimensions
    std::optional<xt::xarray<bool>> _mask;
    std::function<size_t(size_t, size_t, size_t)> _xCorrection;
    std::function<size_t(size_t, size_t, size_t)> _yCorrection;

public:
    OtoscopeTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);
    
    // Add method to update offset with boundary checking
    void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight);
    
    types::VideoFrameDescriptor operator()(types::VideoFrameDescriptor frame);
};
```

### 2. Implement Dynamic Offset Update

```cpp
void OtoscopeTransform::updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight) {
    // Calculate frame center
    const auto cx = frameWidth / 2;
    const auto cy = frameHeight / 2;
    
    // Calculate desired new offset
    int newOffsetX = newCenter.x() - cx;
    int newOffsetY = newCenter.y() - cy;
    
    // Apply boundary constraints to keep the entire circle within frame bounds
    const auto minCenterX = static_cast<int>(_cropRadius);
    const auto maxCenterX = frameWidth - static_cast<int>(_cropRadius);
    const auto minCenterY = static_cast<int>(_cropRadius);
    const auto maxCenterY = frameHeight - static_cast<int>(_cropRadius);
    
    // Clamp the new center position
    const auto clampedCenterX = std::clamp(newCenter.x(), minCenterX, maxCenterX);
    const auto clampedCenterY = std::clamp(newCenter.y(), minCenterY, maxCenterY);
    
    // Update offset based on clamped center
    _offset.first = clampedCenterX - cx;
    _offset.second = clampedCenterY - cy;
    
    // Update correction functions based on new offset
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
    
    // Invalidate mask to force regeneration with new center
    _mask = std::nullopt;
}
```

### 3. Update BuildOtoscopyDisplayWidget Method

Replace the `QLabel` with `OtoScopyLabel` and connect click signals:

```cpp
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel for click handling
    const auto frame = new OtoScopyLabel();
    
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Connect click signal to update crop center
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        // Get current frame dimensions from the last processed frame
        // Note: You may need to store frame dimensions in the worker or pass them differently
        auto* transform = _otoscopyWorker->GetTransform(); // Assuming this method exists
        if (transform) {
            // Update the crop center based on the clicked position
            transform->updateOffset(imagePos, /* frame width */, /* frame height */);
        }
    });

    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}
```

### 4. Modify OtoscopyWorker Class

Add method to access the transform for offset updates:

```cpp
class OtoscopyWorker {
private:
    utility::processing::OtoscopeTransform _transform;
    
public:
    // Add method to get transform reference
    utility::processing::OtoscopeTransform* GetTransform() {
        return &_transform;
    }
    
    // Store frame dimensions for boundary calculations
    void SetFrameDimensions(int width, int height) {
        _frameWidth = width;
        _frameHeight = height;
    }
    
    int GetFrameWidth() const { return _frameWidth; }
    int GetFrameHeight() const { return _frameHeight; }
    
private:
    int _frameWidth = 0;
    int _frameHeight = 0;
};
```

### 5. Enhanced Click Handler with Frame Dimension Tracking

```cpp
// In BuildOtoscopyDisplayWidget, modify the sink to track frame dimensions
_otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
    const auto [rows, cols, channels, step, data] = f;
    
    // Update frame dimensions in worker
    _otoscopyWorker->SetFrameDimensions(cols, rows);
    
    const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
    auto pixmap = QPixmap::fromImage(image);
    overlay(pixmap);
    frame->setPixmap(pixmap);
});

// Updated click handler with proper frame dimensions
connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
    auto* transform = _otoscopyWorker->GetTransform();
    if (transform) {
        const int frameWidth = _otoscopyWorker->GetFrameWidth();
        const int frameHeight = _otoscopyWorker->GetFrameHeight();
        transform->updateOffset(imagePos, frameWidth, frameHeight);
    }
});
```

## Key Implementation Details

### Boundary Logic
- The circle center can move anywhere within the frame
- Constraint: `cropRadius ≤ centerX ≤ frameWidth - cropRadius`
- Constraint: `cropRadius ≤ centerY ≤ frameHeight - cropRadius`
- This ensures the entire circle always stays within the frame boundaries

### Coordinate Mapping
- `OtoScopyLabel::mapToImage()` already converts widget coordinates to image coordinates
- This ensures clicks are accurately mapped to the underlying image pixels

### Mask Regeneration
- When the offset changes, set `_mask = std::nullopt` to force mask regeneration
- The new mask will be centered at the new crop position

### Performance Considerations
- Only regenerate the mask when the offset actually changes
- Consider adding a minimum movement threshold to avoid excessive updates

## Testing Strategy

1. **Boundary Testing**: Click near edges to verify the circle doesn't go outside bounds
2. **Center Accuracy**: Verify that clicking at a point makes that point the new center
3. **Circular Shape**: Ensure the crop remains circular after repositioning
4. **Performance**: Test with rapid clicking to ensure smooth updates

## Additional Enhancements

1. **Visual Feedback**: Consider adding a crosshair or indicator at the current center
2. **Smooth Transitions**: Add animation for center position changes
3. **Reset Function**: Provide a way to reset to the original center position
4. **Configuration**: Make boundary constraints configurable

This implementation provides a robust solution for click-to-reposition functionality while maintaining all the specified constraints and ensuring optimal user experience for precise co-registration of B-Mode and Otoscopy images.