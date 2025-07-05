#include "RemoteOssiviewRoutine.h"
#include "OtoScopyLabel.h"
#include "OtoscopyWorker.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QDebug>
#include <app/Panel.h>
#include <resource/OtoscopyOverlay.h>
#include <config/VisualizationCalibration.h>

// Updated BuildOtoscopyDisplayWidget method with click-to-reposition functionality
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel for click handling
    const auto frame = new OtoScopyLabel();
    
    // OPTION 1: Use enhanced sink with automatic frame tracking
    _otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        
        // Create QImage and apply overlay
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Create transform for click-to-reposition functionality
    // You may need to adjust these values based on your specific requirements
    const uint32_t cropRadius = 150;  // Adjust based on your needs
    const uint32_t maskRadius = 130;  // Adjust based on your needs
    _otoscopyWorker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);

    // Add click handling for crop repositioning
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        auto* transform = _otoscopyWorker->GetTransform();
        if (transform && _otoscopyWorker->HasFrameDimensions()) {
            const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
            
            // Update the crop center based on the clicked position
            transform->updateOffset(imagePos, frameWidth, frameHeight);
            
            // Optional: Log the new center for debugging
            const auto newCenter = transform->getCurrentCenter(frameWidth, frameHeight);
            qDebug() << "Crop center updated to:" << newCenter.first << "," << newCenter.second;
        } else {
            qDebug() << "Cannot update crop center - transform or frame dimensions not available";
        }
    });

    // Optional: Add mouse tracking for hover effects or real-time feedback
    connect(frame, &OtoScopyLabel::mouseMoved, this, [this](const QPoint& imagePos) {
        // Optional: Show preview of where the crop would move
        // This could update a crosshair overlay or status display
        Q_UNUSED(imagePos)
        // You can implement hover effects here if needed
    });

    // Optional: Add wheel event handling for zoom or other features
    connect(frame, &OtoScopyLabel::wheelMoved, this, [this](int delta, const QPoint& imagePos) {
        // Optional: Implement zoom functionality
        Q_UNUSED(delta)
        Q_UNUSED(imagePos)
        // You can implement zoom or other wheel-based features here
    });

    // Enable the otoscopy worker
    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    // Create and return the panel
    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}

// Alternative version with more robust error handling and validation
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidgetWithValidation()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    const auto frame = new OtoScopyLabel();
    
    // Enhanced sink with error handling
    _otoscopyWorker->SetSinkWithFrameTracking([frame, overlay](utility::processing::types::VideoFrameDescriptor f) {
        try {
            const auto [rows, cols, channels, step, data] = f;
            
            // Validate frame data
            if (rows == 0 || cols == 0 || data.empty()) {
                qWarning() << "Invalid frame data received";
                return;
            }
            
            const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
            if (image.isNull()) {
                qWarning() << "Failed to create QImage from frame data";
                return;
            }
            
            auto pixmap = QPixmap::fromImage(image);
            overlay(pixmap);
            frame->setPixmap(pixmap);
        } catch (const std::exception& e) {
            qWarning() << "Error processing frame:" << e.what();
        }
    });

    // Create transform with validation
    const uint32_t cropRadius = 150;  // Adjust based on your needs
    const uint32_t maskRadius = 130;  // Adjust based on your needs
    
    try {
        _otoscopyWorker->CreateTransform(std::make_pair(0, 0), cropRadius, maskRadius);
    } catch (const std::exception& e) {
        qWarning() << "Failed to create transform:" << e.what();
    }

    // Enhanced click handling with validation
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        try {
            auto* transform = _otoscopyWorker->GetTransform();
            if (!transform) {
                qWarning() << "Transform not available for crop repositioning";
                return;
            }
            
            if (!_otoscopyWorker->HasFrameDimensions()) {
                qWarning() << "Frame dimensions not available, cannot reposition crop";
                return;
            }
            
            const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
            
            // Validate click position is within reasonable bounds
            if (imagePos.x() < 0 || imagePos.x() >= frameWidth || 
                imagePos.y() < 0 || imagePos.y() >= frameHeight) {
                qWarning() << "Click position outside frame bounds:" << imagePos;
                return;
            }
            
            // Get old center for comparison
            const auto oldCenter = transform->getCurrentCenter(frameWidth, frameHeight);
            
            // Update the crop center
            transform->updateOffset(imagePos, frameWidth, frameHeight);
            
            // Get new center to verify the change
            const auto newCenter = transform->getCurrentCenter(frameWidth, frameHeight);
            
            qDebug() << "Crop center changed from (" << oldCenter.first << "," << oldCenter.second 
                     << ") to (" << newCenter.first << "," << newCenter.second << ")";
            
            // Optional: Emit signal for other components that might need to know about the change
            // emit cropCenterChanged(QPoint(newCenter.first, newCenter.second));
        } catch (const std::exception& e) {
            qWarning() << "Error handling click event:" << e.what();
        }
    });

    // Enable the otoscopy worker
    if (!_otoscopyWorker->Enable()) {
        qWarning() << "Failed to enable otoscopy worker";
    }

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}

// Alternative version using traditional SetSink method (if you prefer not to use the enhanced sink)
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidgetTraditional()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    const auto frame = new OtoScopyLabel();
    
    // Use traditional sink method with manual frame tracking
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        
        // Manually track frame dimensions
        _otoscopyWorker->SetFrameDimensions(cols, rows);
        
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Create transform
    const uint32_t cropRadius = 150;
    const uint32_t maskRadius = 130;
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