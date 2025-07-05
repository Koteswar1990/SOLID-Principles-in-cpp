# Complete Integration Guide: OtoscopyWorker with Click-to-Reposition

## Overview

This is a complete integration package for adding click-to-reposition functionality to your existing OtoscopyWorker class. The enhancement allows users to click anywhere on the otoscopy image to move the circular crop center to that location.

## ✅ **Key Feature: Constructor Unchanged**

Your existing OtoscopyWorker constructor remains **exactly the same** - no changes needed to any existing constructor calls in your project!

## 📁 Files Included

### Core Enhanced Classes
1. **`OtoscopyWorker.h`** - Enhanced header with new methods for transform and frame tracking
2. **`OtoscopyWorker.cpp`** - Enhanced implementation with click-to-reposition support
3. **`OtoscopeTransform.h`** - Enhanced transform class with dynamic offset updates
4. **`OtoscopeTransform.cpp`** - Transform implementation with boundary constraints
5. **`OtoScopyLabel.h`** - Clickable label with coordinate mapping
6. **`OtoScopyLabel.cpp`** - Click handling and mouse event processing

### Integration Examples
7. **`RemoteOssiviewRoutine.cpp`** - Updated BuildOtoscopyDisplayWidget method
8. **`CMakeLists.txt`** - Build configuration for the enhanced classes

### Documentation
9. **`COMPLETE_INTEGRATION_README.md`** - This comprehensive guide
10. **`CONSTRUCTOR_UNCHANGED_SUMMARY.md`** - Quick reference for key changes
11. **`otoscopy_worker_integration_summary.md`** - Detailed integration steps

## 🚀 Quick Start

### Step 1: Replace Your Existing Files

Replace your existing files with the enhanced versions:

```bash
# Replace your OtoscopyWorker files
cp OtoscopyWorker.h /path/to/your/project/
cp OtoscopyWorker.cpp /path/to/your/project/

# Replace your OtoscopeTransform files
cp OtoscopeTransform.h /path/to/your/project/
cp OtoscopeTransform.cpp /path/to/your/project/

# Add the new OtoScopyLabel files
cp OtoScopyLabel.h /path/to/your/project/
cp OtoScopyLabel.cpp /path/to/your/project/
```

### Step 2: Update Your Display Widget

In your `RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()` method:

```cpp
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel
    const auto frame = new OtoScopyLabel();
    
    // Use enhanced sink with automatic frame tracking
    _otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Create transform for click-to-reposition
    const uint32_t cropRadius = 150;  // Adjust based on your needs
    const uint32_t maskRadius = 130;  // Adjust based on your needs
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

### Step 3: Update Your Build System

Add the new files to your CMakeLists.txt:

```cmake
# Add to your existing source files
set(YOUR_PROJECT_SOURCES
    # ... your existing sources ...
    OtoscopyWorker.cpp
    OtoscopeTransform.cpp
    OtoScopyLabel.cpp
)

# Make sure you have Qt6 Widgets
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Link Qt6 Widgets
target_link_libraries(your_target
    # ... your existing links ...
    Qt6::Core
    Qt6::Widgets
)
```

### Step 4: Test the Integration

1. **Compile** your project
2. **Run** your application
3. **Click** anywhere on the otoscopy image
4. **Verify** the crop center moves to the clicked location

## 🔧 Detailed File Descriptions

### OtoscopyWorker.h / OtoscopyWorker.cpp
**Enhanced worker class with click-to-reposition support**

**Key Changes:**
- ✅ Constructor remains unchanged
- ✅ Added transform storage and access methods
- ✅ Added thread-safe frame dimension tracking
- ✅ Added enhanced sink with automatic frame tracking

**New Methods:**
```cpp
// Transform management
utility::processing::OtoscopeTransform* GetTransform();
void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform);
void CreateTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);
bool HasTransform() const;

// Frame dimension tracking
void SetFrameDimensions(int width, int height);
std::pair<int, int> GetFrameDimensions() const;
bool HasFrameDimensions() const;

// Enhanced sink
void SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink);
```

### OtoscopeTransform.h / OtoscopeTransform.cpp
**Enhanced transform class with dynamic offset updates**

**Key Changes:**
- ✅ Added `updateOffset()` method for click-to-reposition
- ✅ Added boundary constraint checking
- ✅ Added center position calculation methods
- ✅ Added mask invalidation for real-time updates

**New Methods:**
```cpp
// Dynamic offset updates
void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight);
std::pair<int, int> getCurrentOffset() const;
std::pair<int, int> getCurrentCenter(int frameWidth, int frameHeight) const;

// Radius management
uint32_t getCropRadius() const;
uint32_t getMaskRadius() const;
void setCropRadius(uint32_t radius);
void setMaskRadius(uint32_t radius);
```

### OtoScopyLabel.h / OtoScopyLabel.cpp
**Clickable label with coordinate mapping**

**Features:**
- ✅ Accurate coordinate mapping from widget to image coordinates
- ✅ Mouse click event handling
- ✅ Mouse move event tracking
- ✅ Wheel event support
- ✅ Proper aspect ratio handling

**Key Methods:**
```cpp
// Coordinate mapping
QPoint mapToImage(const QPoint& widgetPos) const;
QPoint mapFromImage(const QPoint& imagePos) const;

// Configuration
void setClickEnabled(bool enabled);
bool isClickEnabled() const;
```

**Signals:**
```cpp
void clickedAt(const QPoint& imagePos);
void mouseMoved(const QPoint& imagePos);
void wheelMoved(int delta, const QPoint& imagePos);
```

## 🎯 Integration Options

### Option 1: Enhanced Sink (Recommended)
```cpp
_otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](auto f) {
    // Automatic frame tracking included
    // Your existing sink code here
});
```

### Option 2: Traditional Sink
```cpp
_otoscopyWorker->SetSink([frame, overlay, this](auto f) {
    // Manual frame tracking
    _otoscopyWorker->SetFrameDimensions(f.columns, f.rows);
    // Your existing sink code here
});
```

### Option 3: With Validation
```cpp
_otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](auto f) {
    try {
        // Validate frame data
        if (f.rows == 0 || f.cols == 0) return;
        
        // Your processing code here
    } catch (const std::exception& e) {
        qWarning() << "Frame processing error:" << e.what();
    }
});
```

## 📋 Required Dependencies

### Qt Components
- Qt6::Core
- Qt6::Widgets

### C++ Libraries
- xtensor (for mask operations)
- Your existing utility libraries

### Headers to Include
```cpp
#include <memory>
#include <functional>
#include <mutex>
#include <utility>
#include <algorithm>
#include <QPoint>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
```

## 🔬 Testing Checklist

### Basic Functionality
- [ ] Project compiles without errors
- [ ] All existing OtoscopyWorker usage continues to work
- [ ] Otoscopy image displays correctly
- [ ] Click events are received and processed

### Click-to-Reposition
- [ ] Click anywhere on otoscopy image moves crop center
- [ ] Crop center moves to clicked location
- [ ] Crop maintains circular shape
- [ ] Boundary constraints work (crop doesn't go outside frame)

### Edge Cases
- [ ] Click near edges respects boundary constraints
- [ ] Rapid clicking doesn't cause crashes
- [ ] Frame dimension changes are handled correctly
- [ ] Transform updates work in real-time

### Performance
- [ ] No noticeable performance degradation
- [ ] Smooth real-time updates
- [ ] No memory leaks
- [ ] Thread-safe operation

## 🐛 Troubleshooting

### Issue: Click doesn't move crop center
**Solution:** Verify transform is created and frame dimensions are available
```cpp
// Check transform
if (!_otoscopyWorker->HasTransform()) {
    _otoscopyWorker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);
}

// Check frame dimensions
if (!_otoscopyWorker->HasFrameDimensions()) {
    qDebug() << "Frame dimensions not available";
}
```

### Issue: Crop goes outside frame boundaries
**Solution:** Ensure updateOffset method includes boundary constraints
```cpp
// In OtoscopeTransform::updateOffset
const auto clampedCenterX = std::clamp(newCenter.x(), minCenterX, maxCenterX);
const auto clampedCenterY = std::clamp(newCenter.y(), minCenterY, maxCenterY);
```

### Issue: Compilation errors
**Solution:** Check all required headers and dependencies
```cpp
// Required includes
#include <QPoint>
#include <QLabel>
#include <QMouseEvent>
#include <mutex>
#include <memory>
#include <functional>
```

## 📈 Performance Optimizations

### 1. Frame Tracking Optimization
```cpp
// Use read-heavy optimized mutex
mutable std::shared_mutex _frameDimMutex;
std::shared_lock<std::shared_mutex> lock(_frameDimMutex);  // For reads
std::unique_lock<std::shared_mutex> lock(_frameDimMutex); // For writes
```

### 2. Transform Caching
```cpp
// Cache transform calculations
if (!_transformCache.has_value()) {
    _transformCache = calculateTransform();
}
```

### 3. Mask Invalidation
```cpp
// Only regenerate mask when needed
if (_maskInvalid) {
    _mask = GenerateMask(rows, cols, radius);
    _maskInvalid = false;
}
```

## 🎨 Visual Enhancements (Optional)

### Add Crosshair Indicator
```cpp
// In OtoScopyLabel::paintEvent
void OtoScopyLabel::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);
    
    // Draw crosshair at current crop center
    QPainter painter(this);
    painter.setPen(QPen(Qt::red, 2));
    painter.drawLine(/* crosshair lines */);
}
```

### Add Click Preview
```cpp
// In mouse move handler
connect(frame, &OtoScopyLabel::mouseMoved, this, [](const QPoint& imagePos) {
    // Show preview circle where crop would move
    frame->setPreviewCenter(imagePos);
});
```

## 📊 Migration Summary

### What Changes
- ✅ OtoscopyWorker class gets new methods
- ✅ OtoscopeTransform class gets updateOffset method
- ✅ Display widget uses OtoScopyLabel instead of QLabel
- ✅ Click handling connects to transform updates

### What Stays the Same
- ✅ OtoscopyWorker constructor unchanged
- ✅ All existing methods work exactly the same
- ✅ Existing otoscopy processing pipeline unchanged
- ✅ No changes to existing constructor calls

### Benefits
- ✅ Zero breaking changes
- ✅ Complete backward compatibility
- ✅ Optional enhancement - add only where needed
- ✅ Thread-safe operation
- ✅ Real-time crop repositioning
- ✅ Boundary protection
- ✅ Accurate coordinate mapping

## 🏁 Final Steps

1. **Replace** your existing OtoscopyWorker and OtoscopeTransform files
2. **Add** the new OtoScopyLabel files to your project
3. **Update** your display widget to use OtoScopyLabel
4. **Add** transform creation and click handling
5. **Update** your build system to include the new files
6. **Test** the integration thoroughly
7. **Deploy** with confidence!

## 📞 Support

If you encounter any issues:
1. Check the troubleshooting section
2. Verify all dependencies are installed
3. Ensure all required headers are included
4. Test with the provided example code

The enhanced OtoscopyWorker is designed to be a drop-in replacement with powerful new capabilities while maintaining complete compatibility with your existing project!

**Happy coding! 🎉**