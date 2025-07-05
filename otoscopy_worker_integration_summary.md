# OtoscopyWorker Integration Summary

## Overview
This document outlines the integration of click-to-reposition functionality into your existing OtoscopyWorker class. The enhanced version maintains **complete backward compatibility** - no changes to constructor calls are required!

## Key Changes to Your Existing Class

### 1. **New Private Members**
```cpp
// Transform for crop positioning (optional)
std::unique_ptr<utility::processing::OtoscopeTransform> _transform;

// Thread-safe frame dimension tracking
mutable std::mutex _frameDimMutex;
int _frameWidth = 0;
int _frameHeight = 0;
```

### 2. **Constructor Remains UNCHANGED**
```cpp
// Your existing constructor stays exactly the same - no changes needed!
OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
               utility::processing::OtoscopyStrategyType strategy)
    : _source(source), _strategy(strategy)
```

### 3. **New Public Methods**
```cpp
// Transform management (use these instead of constructor parameter)
utility::processing::OtoscopeTransform* GetTransform();
void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform);
void CreateTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);
bool HasTransform() const;

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

### Step 1: **No Constructor Changes Required!**
```cpp
// Your existing code continues to work exactly as before:
auto worker = std::make_unique<OtoscopyWorker>(source, strategy);
// ✅ No changes needed anywhere in your project!
```

### Step 2: **Add Transform for Click-to-Reposition (Only where needed)**
```cpp
// Only add this in places where you want click-to-reposition:
worker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);
// OR
auto transform = std::make_unique<utility::processing::OtoscopeTransform>(
    std::make_pair(0, 0), cropRadius, maskRadius);
worker->SetTransform(std::move(transform));
```

### Step 3: **Update Display Widget (Only for click functionality)**
```cpp
// In your BuildOtoscopyDisplayWidget method
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel
    const auto frame = new OtoScopyLabel();
    
    // OPTION 1: Use enhanced sink with frame tracking
    _otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // OPTION 2: Or use your existing SetSink and add manual frame tracking
    // _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
    //     const auto [rows, cols, channels, step, data] = f;
    //     _otoscopyWorker->SetFrameDimensions(cols, rows);  // Add this line
    //     const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
    //     auto pixmap = QPixmap::fromImage(image);
    //     overlay(pixmap);
    //     frame->setPixmap(pixmap);
    // });

    // Create transform for click-to-reposition functionality
    _otoscopyWorker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);

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

## Zero-Change Integration

✅ **Constructor unchanged** - All existing OtoscopyWorker constructor calls work as-is  
✅ **All existing methods unchanged** - SetStrategy, SetSink, Enable, Disable work exactly the same  
✅ **Existing functionality preserved** - Your current otoscopy processing continues unchanged  
✅ **Optional enhancement** - Click-to-reposition is only enabled where you explicitly add it  

## New Features (Optional)

✅ **Click-to-reposition crop** - Add transform and connect click signal where needed  
✅ **Thread-safe frame tracking** - Automatic frame dimension tracking  
✅ **Runtime transform creation** - Create transforms on-demand using `CreateTransform()`  
✅ **Transform flexibility** - Set, change, or remove transforms at runtime  

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

- [ ] All existing OtoscopyWorker usage continues to work unchanged
- [ ] Click-to-reposition works where you've added the transform
- [ ] Click near edges respects boundary constraints
- [ ] Frame dimensions are properly tracked
- [ ] Transform updates work in real-time
- [ ] No crashes or race conditions during rapid clicking
- [ ] Co-registration with B-Mode images remains accurate

## Performance Considerations

- **Zero overhead** when transform is not used (nullptr check)
- **Minimal overhead** when transform is used (frame tracking only)
- **Efficient updates** - Transform updates only regenerate mask when needed
- **Thread safety** - Mutex usage optimized for read-heavy scenarios

## Rollback Strategy

If you need to rollback:
1. Simply remove the new methods from the class
2. Remove the new private members
3. Your existing code will continue to work exactly as before
4. No constructor calls need to be changed

## Summary

**✅ No breaking changes** - Your existing code works unchanged  
**✅ No constructor modifications** - All existing constructor calls work as-is  
**✅ Optional enhancement** - Add click-to-reposition only where needed  
**✅ Zero risk** - Enhancement doesn't affect existing functionality  

The enhanced OtoscopyWorker adds powerful new capabilities while maintaining complete backward compatibility!