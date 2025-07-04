// Enhanced OtoScopyLabel class with additional functionality
class OtoScopyLabel : public QLabel
{
    Q_OBJECT

public:
    explicit OtoScopyLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
        setMouseTracking(true);
        setCursor(Qt::CrossCursor); // Show crosshair cursor to indicate clickable area
    }

    QPoint mapToImage(const QPoint& widgetPos) const  
    {
        // Get the pixmap's displayed rect within the label
        if (pixmap().isNull()) {
            return QPoint(-1, -1); // Invalid point if no pixmap
        }
        
        QRect pixRect = pixmap().rect();
        pixRect.moveCenter(rect().center());

        // Check if the click is within the pixmap area
        if (!pixRect.contains(widgetPos)) {
            return QPoint(-1, -1); // Invalid point if outside pixmap
        }

        // Calculate scaling factors
        double scaleX = (widgetPos.x() - pixRect.left()) * pixmap().width() / pixRect.width();
        double scaleY = (widgetPos.y() - pixRect.top()) * pixmap().height() / pixRect.height();

        return QPoint(static_cast<int>(scaleX), static_cast<int>(scaleY));
    }
    
    // Check if a widget position is within the displayed image bounds
    bool isWithinImage(const QPoint& widgetPos) const {
        return mapToImage(widgetPos) != QPoint(-1, -1);
    }

signals:
    void clickedAt(const QPoint& pos);
    void mouseMoved(const QPoint& pos); // New signal for mouse movement
    void wheelScrolled(int delta);
    void mouseEntered();
    void mouseLeft();

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        emit wheelScrolled(event->angleDelta().y());
    }
    
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            QPoint imagePos = mapToImage(event->pos());
            if (imagePos != QPoint(-1, -1)) {
                emit clickedAt(imagePos);
            }
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override
    {
        QPoint imagePos = mapToImage(event->pos());
        if (imagePos != QPoint(-1, -1)) {
            emit mouseMoved(imagePos);
            
            // Update cursor based on whether we're over the image
            setCursor(Qt::CrossCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        
        QLabel::mouseMoveEvent(event);
    }
    
    void enterEvent(QEnterEvent* event) override
    {
        emit mouseEntered();
        QLabel::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override
    {
        emit mouseLeft();
        setCursor(Qt::ArrowCursor);
        QLabel::leaveEvent(event);
    }
    
    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);
        
        // Optional: Draw additional visual feedback here
        // For example, a crosshair at the current crop center
        drawCropIndicator();
    }

private:
    void drawCropIndicator()
    {
        // Optional: Draw a subtle indicator showing the current crop center
        // This would require access to the current transform state
        // Implementation depends on your UI design requirements
        
        /*
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw a small crosshair at the center or current crop position
        QPen pen(QColor(255, 255, 255, 128), 1, Qt::SolidLine);
        painter.setPen(pen);
        
        QPoint center = rect().center();
        int crossSize = 10;
        
        painter.drawLine(center.x() - crossSize, center.y(), 
                        center.x() + crossSize, center.y());
        painter.drawLine(center.x(), center.y() - crossSize, 
                        center.x(), center.y() + crossSize);
        */
    }
};

// Enhanced version with crop center tracking
class OtoScopyLabelWithCenterTracking : public OtoScopyLabel
{
    Q_OBJECT

private:
    QPoint _currentCropCenter;
    bool _cropCenterValid = false;
    int _cropRadius = 0;

public:
    explicit OtoScopyLabelWithCenterTracking(QWidget* parent = nullptr) 
        : OtoScopyLabel(parent) {}
    
    void setCropCenter(const QPoint& center, int radius) {
        _currentCropCenter = center;
        _cropRadius = radius;
        _cropCenterValid = true;
        update(); // Trigger repaint to show the new center
    }
    
    void clearCropCenter() {
        _cropCenterValid = false;
        update();
    }
    
    QPoint getCurrentCropCenter() const {
        return _currentCropCenter;
    }
    
    bool hasCropCenter() const {
        return _cropCenterValid;
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        OtoScopyLabel::paintEvent(event);
        
        if (_cropCenterValid && !pixmap().isNull()) {
            drawCropCenterIndicator();
        }
    }

private:
    void drawCropCenterIndicator()
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Map crop center from image coordinates to widget coordinates
        QRect pixRect = pixmap().rect();
        pixRect.moveCenter(rect().center());
        
        double scaleX = static_cast<double>(pixRect.width()) / pixmap().width();
        double scaleY = static_cast<double>(pixRect.height()) / pixmap().height();
        
        QPoint widgetCenter(
            pixRect.left() + _currentCropCenter.x() * scaleX,
            pixRect.top() + _currentCropCenter.y() * scaleY
        );
        
        // Draw crosshair at crop center
        QPen pen(QColor(255, 255, 0, 180), 2, Qt::SolidLine);
        painter.setPen(pen);
        
        int crossSize = 15;
        painter.drawLine(widgetCenter.x() - crossSize, widgetCenter.y(), 
                        widgetCenter.x() + crossSize, widgetCenter.y());
        painter.drawLine(widgetCenter.x(), widgetCenter.y() - crossSize, 
                        widgetCenter.x(), widgetCenter.y() + crossSize);
        
        // Optional: Draw circle outline showing crop boundary
        if (_cropRadius > 0) {
            QPen circlePen(QColor(255, 255, 0, 100), 1, Qt::DashLine);
            painter.setPen(circlePen);
            
            int radiusWidget = _cropRadius * scaleX; // Assuming uniform scaling
            painter.drawEllipse(widgetCenter, radiusWidget, radiusWidget);
        }
    }
};