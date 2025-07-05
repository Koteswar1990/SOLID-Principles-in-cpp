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
        
        // NEW: Transform member for crop positioning
        std::unique_ptr<utility::processing::OtoscopeTransform> _transform;
        
        // NEW: Frame dimension tracking
        mutable std::mutex _frameDimMutex;
        int _frameWidth = 0;
        int _frameHeight = 0;

    public:
        // Updated constructor to accept transform
        OtoscopyWorker(std::shared_ptr<utility::processing::VideoSource> source,
                       utility::processing::OtoscopyStrategyType strategy,
                       std::unique_ptr<utility::processing::OtoscopeTransform> transform = nullptr)
            : _source(source), _strategy(strategy), _transform(std::move(transform))
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

        // Move constructor
        OtoscopyWorker(OtoscopyWorker&& other) noexcept
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

        // Move assignment operator
        OtoscopyWorker& operator=(OtoscopyWorker&& other) noexcept
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

        // Existing methods
        void SetStrategy(utility::processing::OtoscopyStrategyType strategy) noexcept
        {
            _strategy = strategy;
        }

        void SetSink(utility::processing::OtoscopySinkType sink) noexcept
        {
            _sink = sink;
        }

        bool Enable() noexcept
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

        bool Disable() noexcept
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
        utility::processing::OtoscopeTransform* GetTransform() 
        {
            return _transform.get();
        }

        void SetTransform(std::unique_ptr<utility::processing::OtoscopeTransform> transform)
        {
            _transform = std::move(transform);
        }

        // NEW: Frame dimension tracking methods
        void SetFrameDimensions(int width, int height) 
        {
            std::lock_guard<std::mutex> lock(_frameDimMutex);
            _frameWidth = width;
            _frameHeight = height;
        }

        int GetFrameWidth() const 
        {
            std::lock_guard<std::mutex> lock(_frameDimMutex);
            return _frameWidth;
        }

        int GetFrameHeight() const 
        {
            std::lock_guard<std::mutex> lock(_frameDimMutex);
            return _frameHeight;
        }

        std::pair<int, int> GetFrameDimensions() const 
        {
            std::lock_guard<std::mutex> lock(_frameDimMutex);
            return {_frameWidth, _frameHeight};
        }

        bool HasFrameDimensions() const 
        {
            std::lock_guard<std::mutex> lock(_frameDimMutex);
            return _frameWidth > 0 && _frameHeight > 0;
        }

        // NEW: Enhanced sink setter that tracks frame dimensions
        void SetSinkWithFrameTracking(std::function<void(utility::processing::types::VideoFrameDescriptor)> userSink)
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
    };
}