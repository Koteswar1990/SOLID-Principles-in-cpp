// Updated BuildOtoscopyDisplayWidget method in RemoteOssiviewRoutine class
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidget()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    
    // Use OtoScopyLabel instead of QLabel for click handling
    const auto frame = new OtoScopyLabel();
    
    // Enhanced sink that tracks frame dimensions and processes frames
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        
        // Update frame dimensions in worker for boundary calculations
        _otoscopyWorker->SetFrameDimensions(cols, rows);
        
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    // Connect click signal to update crop center
    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
        auto* transform = _otoscopyWorker->GetTransform();
        if (transform && _otoscopyWorker->HasFrameDimensions()) {
            const auto [frameWidth, frameHeight] = _otoscopyWorker->GetFrameDimensions();
            
            // Update the crop center based on the clicked position
            transform->updateOffset(imagePos, frameWidth, frameHeight);
            
            // Optional: Log the new center for debugging
            const auto newCenter = transform->getCurrentCenter(frameWidth, frameHeight);
            qDebug() << "Crop center updated to:" << newCenter.first << "," << newCenter.second;
        }
    });

    // Optional: Add mouse tracking for hover effects or real-time feedback
    connect(frame, &OtoScopyLabel::mouseMoved, this, [this](const QPoint& imagePos) {
        // Optional: Show preview of where the crop would move
        // This could update a crosshair overlay or status display
        Q_UNUSED(imagePos)
    });

    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}

// Alternative version with error handling and validation
QWidget* RemoteOssiviewRoutine::BuildOtoscopyDisplayWidgetWithValidation()
{
    const auto overlay = resource::MakeOtoscopyOverlay(config::VisualizationCalibration());
    const auto frame = new OtoScopyLabel();
    
    _otoscopyWorker->SetSink([frame, overlay, this](utility::processing::types::VideoFrameDescriptor f) {
        const auto [rows, cols, channels, step, data] = f;
        _otoscopyWorker->SetFrameDimensions(cols, rows);
        
        const auto image = QImage(data.data(), cols, rows, step, QImage::Format_BGR888).copy();
        auto pixmap = QPixmap::fromImage(image);
        overlay(pixmap);
        frame->setPixmap(pixmap);
    });

    connect(frame, &OtoScopyLabel::clickedAt, this, [this](const QPoint& imagePos) {
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
        
        // Validate click position is within frame bounds
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
    });

    [[maybe_unused]] auto _ = _otoscopyWorker->Enable();

    const auto panel = new app::Panel(app::Panel::Type::Imaging);
    auto layout = new QHBoxLayout(panel);
    layout->addWidget(frame, 0, Qt::AlignCenter);
    return panel;
}