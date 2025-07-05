#include "OtoscopeTransform.h"
#include <utility/processing/types/VideoFrameDescriptor.h>
#include <utility/processing/ImageProcessing.h>
#include <algorithm>

namespace utility::processing {

    OtoscopeTransform::OtoscopeTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius)
        : _offset(offset), _cropRadius(cropRadius), _maskRadius(maskRadius), _mask(std::nullopt)
    {
        updateCorrectionFunctions();
    }

    OtoscopeTransform::OtoscopeTransform(const OtoscopeTransform& other)
        : _offset(other._offset), 
          _cropRadius(other._cropRadius), 
          _maskRadius(other._maskRadius),
          _mask(other._mask),
          _xCorrection(other._xCorrection),
          _yCorrection(other._yCorrection)
    {
    }

    OtoscopeTransform::OtoscopeTransform(OtoscopeTransform&& other) noexcept
        : _offset(std::move(other._offset)),
          _cropRadius(other._cropRadius),
          _maskRadius(other._maskRadius),
          _mask(std::move(other._mask)),
          _xCorrection(std::move(other._xCorrection)),
          _yCorrection(std::move(other._yCorrection))
    {
    }

    OtoscopeTransform& OtoscopeTransform::operator=(const OtoscopeTransform& other)
    {
        if (this != &other)
        {
            _offset = other._offset;
            _cropRadius = other._cropRadius;
            _maskRadius = other._maskRadius;
            _mask = other._mask;
            _xCorrection = other._xCorrection;
            _yCorrection = other._yCorrection;
        }
        return *this;
    }

    OtoscopeTransform& OtoscopeTransform::operator=(OtoscopeTransform&& other) noexcept
    {
        if (this != &other)
        {
            _offset = std::move(other._offset);
            _cropRadius = other._cropRadius;
            _maskRadius = other._maskRadius;
            _mask = std::move(other._mask);
            _xCorrection = std::move(other._xCorrection);
            _yCorrection = std::move(other._yCorrection);
        }
        return *this;
    }

    void OtoscopeTransform::updateCorrectionFunctions()
    {
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
    }

    types::VideoFrameDescriptor OtoscopeTransform::operator()(types::VideoFrameDescriptor frame)
    {
        const auto [cx, cy] = std::make_pair(frame.columns / 2, frame.rows / 2);
        const auto offsetX = _xCorrection(0, cx + _offset.first, frame.columns);
        const auto offsetY = _yCorrection(0, cy + _offset.second, frame.rows);
        const auto center = std::make_pair(offsetX, offsetY);
        
        auto trimmed = TrimImage(std::move(frame), center, _cropRadius, _cropRadius);
        
        if (!_mask.has_value()) {
            _mask = GenerateMask(trimmed.rows, trimmed.columns, _maskRadius);
        }
        
        auto maskedImage = MaskImage(_mask.value(), trimmed);
        return maskedImage;
    }

    // NEW: Update offset dynamically for click-to-reposition
    void OtoscopeTransform::updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight)
    {
        const auto cx = frameWidth / 2;
        const auto cy = frameHeight / 2;
        
        // Apply boundary constraints
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

    // NEW: Get current offset
    std::pair<int, int> OtoscopeTransform::getCurrentOffset() const
    {
        return _offset;
    }

    // NEW: Get current center position
    std::pair<int, int> OtoscopeTransform::getCurrentCenter(int frameWidth, int frameHeight) const
    {
        const auto cx = frameWidth / 2;
        const auto cy = frameHeight / 2;
        return {cx + _offset.first, cy + _offset.second};
    }

    // NEW: Get crop radius
    uint32_t OtoscopeTransform::getCropRadius() const
    {
        return _cropRadius;
    }

    // NEW: Get mask radius
    uint32_t OtoscopeTransform::getMaskRadius() const
    {
        return _maskRadius;
    }

    // NEW: Set crop radius
    void OtoscopeTransform::setCropRadius(uint32_t radius)
    {
        _cropRadius = radius;
        _mask = std::nullopt; // Force mask regeneration
    }

    // NEW: Set mask radius
    void OtoscopeTransform::setMaskRadius(uint32_t radius)
    {
        _maskRadius = radius;
        _mask = std::nullopt; // Force mask regeneration
    }

    // NEW: Force mask regeneration
    void OtoscopeTransform::invalidateMask()
    {
        _mask = std::nullopt;
    }

    // Helper methods - you need to implement these based on your existing code
    types::VideoFrameDescriptor OtoscopeTransform::TrimImage(types::VideoFrameDescriptor frame, 
                                                           std::pair<size_t, size_t> center, 
                                                           uint32_t cropWidth, 
                                                           uint32_t cropHeight)
    {
        // Implementation depends on your existing TrimImage function
        // This is a placeholder - replace with your actual implementation
        return utility::processing::TrimImage(std::move(frame), center, cropWidth, cropHeight);
    }

    xt::xarray<bool> OtoscopeTransform::GenerateMask(size_t rows, size_t cols, uint32_t radius)
    {
        // Implementation depends on your existing GenerateMask function
        // This is a placeholder - replace with your actual implementation
        return utility::processing::GenerateMask(rows, cols, radius);
    }

    types::VideoFrameDescriptor OtoscopeTransform::MaskImage(const xt::xarray<bool>& mask, 
                                                           const types::VideoFrameDescriptor& frame)
    {
        // Implementation depends on your existing MaskImage function
        // This is a placeholder - replace with your actual implementation
        return utility::processing::MaskImage(mask, frame);
    }

} // namespace utility::processing