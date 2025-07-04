// Updated OtoscopyWorker class with transform access and frame tracking
class OtoscopyWorker {
private:
    utility::processing::OtoscopeTransform _transform;
    // Add any other existing private members your worker has
    // ...
    
    // Frame dimension tracking
    mutable std::mutex _frameDimMutex;
    int _frameWidth = 0;
    int _frameHeight = 0;
    
public:
    // Keep your existing constructor
    OtoscopyWorker(/* your existing parameters */, utility::processing::OtoscopeTransform transform)
        : _transform(std::move(transform))
    {
        // Keep your existing initialization
    }
    
    // Method to get transform reference for offset updates
    utility::processing::OtoscopeTransform* GetTransform() {
        return &_transform;
    }
    
    // Thread-safe methods to store and retrieve frame dimensions
    void SetFrameDimensions(int width, int height) {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        _frameWidth = width;
        _frameHeight = height;
    }
    
    int GetFrameWidth() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameWidth;
    }
    
    int GetFrameHeight() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameHeight;
    }
    
    std::pair<int, int> GetFrameDimensions() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return {_frameWidth, _frameHeight};
    }
    
    // Method to check if frame dimensions are available
    bool HasFrameDimensions() const {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameWidth > 0 && _frameHeight > 0;
    }
    
    // Keep all your existing methods (Enable, SetSink, etc.)
    // Just make sure the processing pipeline uses _transform
    
    // Example of how your processing method might look:
    void ProcessFrame(types::VideoFrameDescriptor frame) {
        // Update frame dimensions
        SetFrameDimensions(frame.columns, frame.rows);
        
        // Apply the transform (which now supports dynamic offset)
        auto processedFrame = _transform(std::move(frame));
        
        // Continue with your existing processing pipeline
        // ...
    }
};