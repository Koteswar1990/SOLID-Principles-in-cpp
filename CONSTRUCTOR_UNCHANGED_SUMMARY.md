# ✅ OtoscopyWorker Integration - Constructor Unchanged

## Key Point: **No Constructor Changes Required!**

Your existing OtoscopyWorker constructor remains **exactly the same**:

```cpp
// ✅ This stays unchanged - works everywhere in your project
OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
               utility::processing::OtoscopyStrategyType strategy)
```

## What You Add (New Private Members)

```cpp
private:
    // Your existing members stay the same...
    
    // ✅ Add these new members:
    std::unique_ptr<utility::processing::OtoscopeTransform> _transform;
    mutable std::mutex _frameDimMutex;
    int _frameWidth = 0;
    int _frameHeight = 0;
```

## What You Add (New Methods)

```cpp
public:
    // ✅ Add these new methods:
    
    // Transform management
    utility::processing::OtoscopeTransform* GetTransform();
    void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform);
    void CreateTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);
    bool HasTransform() const;
    
    // Frame tracking
    void SetFrameDimensions(int width, int height);
    std::pair<int, int> GetFrameDimensions() const;
    bool HasFrameDimensions() const;
    
    // Enhanced sink
    void SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink);
```

## How to Enable Click-to-Reposition (Only Where Needed)

```cpp
// 1. Create transform where you want click-to-reposition
_otoscopyWorker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);

// 2. Use OtoScopyLabel instead of QLabel
const auto frame = new OtoScopyLabel();

// 3. Add click handling
connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
    auto* transform = _otoscopyWorker->GetTransform();
    if (transform && _otoscopyWorker->HasFrameDimensions()) {
        const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
        transform->updateOffset(imagePos, frameWidth, frameHeight);
    }
});
```

## Migration Steps

### Step 1: **Drop-in Replacement** (Zero Risk)
1. Add the new private members to your OtoscopyWorker class
2. Add the new methods to your OtoscopyWorker class  
3. Update move constructor and assignment operator
4. Compile - everything should work exactly as before

### Step 2: **Add Click-to-Reposition** (Where Needed)
1. Only in display widgets where you want this feature
2. Replace QLabel with OtoScopyLabel
3. Add transform creation and click handling
4. Test the new functionality

## Benefits

✅ **Zero Breaking Changes** - All existing code works unchanged  
✅ **No Constructor Updates** - Constructor calls work as-is throughout your project  
✅ **Optional Enhancement** - Click-to-reposition only where you add it  
✅ **Thread Safe** - Frame tracking is thread-safe  
✅ **Boundary Protected** - Crop never goes outside frame  

## Summary

**The enhanced OtoscopyWorker is a perfect drop-in replacement:**
- Constructor signature unchanged
- All existing methods work exactly the same  
- No changes needed to existing constructor calls
- New features are completely optional
- Add click-to-reposition only where you want it

**Your existing OtoscopyWorker usage throughout your project continues to work unchanged!**