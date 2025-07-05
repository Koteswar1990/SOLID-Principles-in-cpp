#include "OtoscopyWorker.h"
#include "OtoscopeTransform.h"
#include <utility/logger/Logger.h>
#include <utility/processing/VideoPipeline.h>
#include <utility/processing/VideoSource.h>
#include <utility/processing/types/VideoFrameDescriptor.h>

namespace imaging
{
    OtoscopyWorker::OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
                                   utility::processing::OtoscopyStrategyType strategy)
        : _source(source), _strategy(strategy)
    {
        _sink = utility::processing::OtoscopySinkType{};
        _handler = [this](std::exception_ptr ptr) {
            try
            {
                if (ptr)
                {
                    std::rethrow_exception(ptr);
                }
            }
            catch (const std::exception& e)
            {
                utility::logger::Error(e.what());
            }
        };
    }

    OtoscopyWorker::OtoscopyWorker(OtoscopyWorker&& other) noexcept
        : _sink{other._sink}, 
          _pipeline{std::move(other._pipeline)}, 
          _source{other._source}, 
          _strategy{other._strategy},
          _handler{other._handler},
          _transform{std::move(other._transform)},
          _frameWidth{other._frameWidth},
          _frameHeight{other._frameHeight}
    {
    }

    OtoscopyWorker& OtoscopyWorker::operator=(OtoscopyWorker&& other) noexcept
    {
        if (this != &other)
        {
            _sink = std::move(other._sink);
            _pipeline = std::move(other._pipeline);
            _source = std::move(other._source);
            _strategy = std::move(other._strategy);
            _handler = std::move(other._handler);
            _transform = std::move(other._transform);
            _frameWidth = other._frameWidth;
            _frameHeight = other._frameHeight;
        }
        return *this;
    }

    void OtoscopyWorker::SetStrategy(utility::processing::OtoscopyStrategyType strategy) noexcept
    {
        _strategy = strategy;
    }

    void OtoscopyWorker::SetSink(utility::processing::OtoscopySinkType sink) noexcept
    {
        _sink = sink;
    }

    bool OtoscopyWorker::Enable() noexcept
    {
        if (_pipeline)
        {
            _pipeline.release();
        }

        try
        {
            _pipeline = std::make_unique<utility::processing::VideoPipeline>(_source, _strategy, _sink, _handler);
            _pipeline->start();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool OtoscopyWorker::Disable() noexcept
    {
        if (_pipeline == nullptr)
        {
            return true;
        }
        try
        {
            _pipeline->stop();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // NEW: Transform access methods
    utility::processing::OtoscopeTransform* OtoscopyWorker::GetTransform()
    {
        return _transform.get();
    }

    const utility::processing::OtoscopeTransform* OtoscopyWorker::GetTransform() const
    {
        return _transform.get();
    }

    void OtoscopyWorker::SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform)
    {
        _transform = std::move(transform);
    }

    void OtoscopyWorker::CreateTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius)
    {
        _transform = std::make_unique<utility::processing::OtoscopeTransform>(offset, cropRadius, maskRadius);
    }

    bool OtoscopyWorker::HasTransform() const
    {
        return _transform != nullptr;
    }

    // NEW: Frame dimension tracking methods
    void OtoscopyWorker::SetFrameDimensions(int width, int height)
    {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        _frameWidth = width;
        _frameHeight = height;
    }

    int OtoscopyWorker::GetFrameWidth() const
    {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameWidth;
    }

    int OtoscopyWorker::GetFrameHeight() const
    {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameHeight;
    }

    std::pair<int, int> OtoscopyWorker::GetFrameDimensions() const
    {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return {_frameWidth, _frameHeight};
    }

    bool OtoscopyWorker::HasFrameDimensions() const
    {
        std::lock_guard<std::mutex> lock(_frameDimMutex);
        return _frameWidth > 0 && _frameHeight > 0;
    }

    // NEW: Enhanced sink setter that tracks frame dimensions
    void OtoscopyWorker::SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink)
    {
        _sink = [this, userSink](utility::processing::types::VideoFrameDescriptor frame) {
            // Track frame dimensions automatically
            SetFrameDimensions(frame.columns, frame.rows);
            
            // Call user's sink function
            if (userSink) {
                userSink(frame);
            }
        };
    }
}