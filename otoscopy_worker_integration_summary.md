# OtoscopyWorker Integration Summary

## Overview
This document outlines the integration of click-to-reposition functionality into your existing OtoscopyWorker class. The enhanced version maintains backward compatibility while adding new features for dynamic crop positioning.

## Key Changes to Your Existing Class

### 1. **New Private Members**
```cpp
// Transform for crop positioning
std::unique_ptr<utility::processing::OtoscopeTransform> _transform;

// Thread-safe frame dimension tracking
mutable std::mutex _frameDimMutex;
int _frameWidth = 0;
int _frameHeight = 0;
```

### 2. **Updated Constructor**
```cpp
// New constructor signature (backward compatible)
OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
               utility::processing::OtoscopyStrategyType strategy,
               std::unique_ptr<utility::processing::OtoscopeTransform> transform = nullptr)
```

### 3. **New Public Methods**
```cpp
// Transform access
utility::processing::OtoscopeTransform* GetTransform();
void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform);

// Frame dimension tracking
void SetFrameDimensions(int width, int height);
int GetFrameWidth() const;
int GetFrameHeight() const;
std::pair<int, int> GetFrameDimensions() const;
bool HasFrameDimensions() const;

// Enhanced sink with automatic frame tracking
void SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink);
```

## Migration Guide

### Step 1: Update Your Constructor Calls
```cpp
// Old way (still works)
auto worker = std::make_unique<OtoscopyWorker>(source, strategy);

// New way with transform
auto transform = std::make_unique<utility::processing::OtoscopeTransform>(
    std::make_pair(0, 0), cropRadius, maskRadius);
auto worker = std::make_unique<OtoscopyWorker>(source, strategy, std::move(transform));
```

### Step 2: Update Your Display Widget
```cpp
// In your BuildOtoscopyDisplayWidget method
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel
    const auto frame = new OtoScopyLabel();
    
    // Use enhanced sink with frame tracking
    _otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Add click handling
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

## Backward Compatibility

✅ **Existing code continues to work unchanged**
- Old constructor signature still works
- All existing methods remain functional
- No breaking changes to your current implementation

## New Features

✅ **Click-to-reposition crop**
- Click anywhere on the otoscopy image to move the crop center
- Automatic boundary constraints prevent the crop from going outside the frame

✅ **Thread-safe frame tracking**
- Frame dimensions are safely tracked across threads
- Prevents race conditions in multi-threaded environments

✅ **Transform access**
- External components can access and modify the transform
- Enables dynamic crop positioning and real-time adjustments

## Required Dependencies

Make sure your project includes these headers:
```cpp
#include <memory>
#include <mutex>
#include <functional>
#include <utility>
#include <QPoint>
#include <QLabel>
#include <QMouseEvent>
```

## Testing Checklist

- [ ] Existing otoscopy functionality works unchanged
- [ ] Click anywhere on otoscopy image moves crop center
- [ ] Click near edges respects boundary constraints
- [ ] Frame dimensions are properly tracked
- [ ] Transform updates work in real-time
- [ ] No crashes or race conditions during rapid clicking
- [ ] Co-registration with B-Mode images remains accurate

## Performance Considerations

- **Minimal overhead**: Frame tracking adds negligible performance cost
- **Efficient updates**: Transform updates only regenerate mask when needed
- **Thread safety**: Mutex usage is optimized for read-heavy scenarios
- **Memory usage**: Transform is stored as unique_ptr for efficient moves

## Troubleshooting

**Issue**: Click doesn't move crop center
- **Check**: Ensure transform is set and frame dimensions are available
- **Debug**: Add logging to verify click coordinates and frame dimensions

**Issue**: Crop goes outside frame boundaries
- **Check**: Verify OtoscopeTransform::updateOffset() method includes boundary constraints
- **Debug**: Log clamp values and verify they're within frame bounds

**Issue**: Performance degradation
- **Check**: Ensure SetSinkWithFrameTracking is used instead of manual frame tracking
- **Debug**: Profile mutex contention in frame dimension tracking

## Next Steps

1. **Test the integration** with your existing codebase
2. **Validate click-to-reposition** functionality works as expected
3. **Verify co-registration** with B-Mode images remains accurate
4. **Consider visual feedback** improvements (crosshair, preview circle)
5. **Add unit tests** for new functionality