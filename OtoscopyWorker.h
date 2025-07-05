#pragma once

#include <memory>
#include <functional>
#include <mutex>
#include <utility>

// Forward declarations
namespace utility::processing {
    class VideoSource;
    class VideoPipeline;
    class OtoscopeTransform;
    enum class OtoscopyStrategyType;
    using OtoscopySinkType = std::function<void(types::VideoFrameDescriptor)>;
    
    namespace types {
        struct VideoFrameDescriptor;
    }
}

namespace imaging
{
    class OtoscopyWorker
    {
    private:
        std::shared_ptr<utility::processing::VideoSource> _source;
        utility::processing::OtoscopyStrategyType _strategy;
        utility::processing::OtoscopySinkType _sink;
        std::unique_ptr<utility::processing::VideoPipeline> _pipeline;
        std::function<void(std::exception_ptr)> _handler;
        
        // NEW: Transform member for crop positioning (optional)
        std::unique_ptr<utility::processing::OtoscopeTransform> _transform;
        
        // NEW: Frame dimension tracking
        mutable std::mutex _frameDimMutex;
        int _frameWidth = 0;
        int _frameHeight = 0;

    public:
        // UNCHANGED: Keep original constructor exactly the same
        OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
                       utility::processing::OtoscopyStrategyType strategy);

        // Move constructor
        OtoscopyWorker(OtoscopyWorker&& other) noexcept;

        // Move assignment operator
        OtoscopyWorker& operator=(OtoscopyWorker&& other) noexcept;

        // Disable copy constructor and copy assignment
        OtoscopyWorker(const OtoscopyWorker&) = delete;
        OtoscopyWorker& operator=(const OtoscopyWorker&) = delete;

        // Destructor
        ~OtoscopyWorker() = default;

        // UNCHANGED: All existing methods remain exactly the same
        void SetStrategy(utility::processing::OtoscopyStrategyType strategy) noexcept;
        void SetSink(utility::processing::OtoscopySinkType sink) noexcept;
        bool Enable() noexcept;
        bool Disable() noexcept;

        // NEW: Transform access methods (use these instead of constructor parameter)
        utility::processing::OtoscopeTransform* GetTransform();
        const utility::processing::OtoscopeTransform* GetTransform() const;
        void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform);

        // NEW: Create and set transform with parameters
        void CreateTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);

        // NEW: Check if transform is available
        bool HasTransform() const;

        // NEW: Frame dimension tracking methods
        void SetFrameDimensions(int width, int height);
        int GetFrameWidth() const;
        int GetFrameHeight() const;
        std::pair<int, int> GetFrameDimensions() const;
        bool HasFrameDimensions() const;

        // NEW: Enhanced sink setter that tracks frame dimensions
        void SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink);
    };
}