#pragma once

#include <utility>
#include <optional>
#include <functional>
#include <cstdint>
#include <QPoint>
#include <xtensor/xarray.hpp>

// Forward declarations
namespace utility::processing::types {
    struct VideoFrameDescriptor;
}

namespace utility::processing {

    class OtoscopeTransform {
    private:
        std::pair<int, int> _offset;
        uint32_t _cropRadius;
        uint32_t _maskRadius;
        mutable std::optional<xt::xarray<bool>> _mask;
        std::function<size_t(size_t, size_t, size_t)> _xCorrection;
        std::function<size_t(size_t, size_t, size_t)> _yCorrection;

        void updateCorrectionFunctions();

    public:
        // Constructor
        OtoscopeTransform(std::pair<int, int> offset, uint32_t cropRadius, uint32_t maskRadius);

        // Copy constructor
        OtoscopeTransform(const OtoscopeTransform& other);

        // Move constructor
        OtoscopeTransform(OtoscopeTransform&& other) noexcept;

        // Copy assignment operator
        OtoscopeTransform& operator=(const OtoscopeTransform& other);

        // Move assignment operator
        OtoscopeTransform& operator=(OtoscopeTransform&& other) noexcept;

        // Destructor
        ~OtoscopeTransform() = default;

        // Main processing operator
        types::VideoFrameDescriptor operator()(types::VideoFrameDescriptor frame);

        // NEW: Update offset dynamically for click-to-reposition
        void updateOffset(const QPoint& newCenter, int frameWidth, int frameHeight);

        // NEW: Get current offset
        std::pair<int, int> getCurrentOffset() const;

        // NEW: Get current center position
        std::pair<int, int> getCurrentCenter(int frameWidth, int frameHeight) const;

        // NEW: Get crop radius
        uint32_t getCropRadius() const;

        // NEW: Get mask radius
        uint32_t getMaskRadius() const;

        // NEW: Set crop radius
        void setCropRadius(uint32_t radius);

        // NEW: Set mask radius
        void setMaskRadius(uint32_t radius);

        // NEW: Force mask regeneration
        void invalidateMask();

    private:
        // Helper methods (you need to implement these based on your existing code)
        types::VideoFrameDescriptor TrimImage(types::VideoFrameDescriptor frame, 
                                            std::pair<size_t, size_t> center, 
                                            uint32_t cropWidth, 
                                            uint32_t cropHeight);
        
        xt::xarray<bool> GenerateMask(size_t rows, size_t cols, uint32_t radius);
        
        types::VideoFrameDescriptor MaskImage(const xt::xarray<bool>& mask, 
                                            const types::VideoFrameDescriptor& frame);
    };

} // namespace utility::processing