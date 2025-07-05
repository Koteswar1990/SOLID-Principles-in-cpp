#include "OtoScopyLabel.h"
#include <QDebug>
#include <QApplication>
#include <QStyle>

OtoScopyLabel::OtoScopyLabel(QWidget* parent)
    : QLabel(parent), _clickEnabled(true)
{
    // Enable mouse tracking for mouse move events
    setMouseTracking(true);
    
    // Set cursor to indicate clickable area
    setCursor(Qt::CrossCursor);
    
    // Set size policy to maintain aspect ratio
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OtoScopyLabel::setClickEnabled(bool enabled)
{
    _clickEnabled = enabled;
    
    // Update cursor based on click state
    if (_clickEnabled) {
        setCursor(Qt::CrossCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

bool OtoScopyLabel::isClickEnabled() const
{
    return _clickEnabled;
}

QPoint OtoScopyLabel::mapToImage(const QPoint& widgetPos) const
{
    if (!pixmap()) {
        return QPoint(-1, -1);
    }
    
    const QRect imageRect = getImageRect();
    const QPixmap* pm = pixmap();
    
    if (!imageRect.contains(widgetPos)) {
        return QPoint(-1, -1);
    }
    
    // Calculate relative position within the image area
    const double relativeX = static_cast<double>(widgetPos.x() - imageRect.x()) / imageRect.width();
    const double relativeY = static_cast<double>(widgetPos.y() - imageRect.y()) / imageRect.height();
    
    // Map to actual image coordinates
    const int imageX = static_cast<int>(relativeX * pm->width());
    const int imageY = static_cast<int>(relativeY * pm->height());
    
    // Clamp to image bounds
    const int clampedX = std::clamp(imageX, 0, pm->width() - 1);
    const int clampedY = std::clamp(imageY, 0, pm->height() - 1);
    
    return QPoint(clampedX, clampedY);
}

QPoint OtoScopyLabel::mapFromImage(const QPoint& imagePos) const
{
    if (!pixmap()) {
        return QPoint(-1, -1);
    }
    
    const QRect imageRect = getImageRect();
    const QPixmap* pm = pixmap();
    
    // Calculate relative position within the image
    const double relativeX = static_cast<double>(imagePos.x()) / pm->width();
    const double relativeY = static_cast<double>(imagePos.y()) / pm->height();
    
    // Map to widget coordinates
    const int widgetX = imageRect.x() + static_cast<int>(relativeX * imageRect.width());
    const int widgetY = imageRect.y() + static_cast<int>(relativeY * imageRect.height());
    
    return QPoint(widgetX, widgetY);
}

double OtoScopyLabel::getScaleFactor() const
{
    if (!pixmap()) {
        return 1.0;
    }
    
    const QRect imageRect = getImageRect();
    const QPixmap* pm = pixmap();
    
    const double scaleX = static_cast<double>(imageRect.width()) / pm->width();
    const double scaleY = static_cast<double>(imageRect.height()) / pm->height();
    
    return std::min(scaleX, scaleY);
}

QSize OtoScopyLabel::getImageSize() const
{
    if (!pixmap()) {
        return QSize(0, 0);
    }
    
    return pixmap()->size();
}

void OtoScopyLabel::mousePressEvent(QMouseEvent* event)
{
    if (!_clickEnabled || event->button() != Qt::LeftButton) {
        QLabel::mousePressEvent(event);
        return;
    }
    
    const QPoint imagePos = mapToImage(event->pos());
    
    if (imagePos.x() >= 0 && imagePos.y() >= 0) {
        emit clickedAt(imagePos);
        qDebug() << "OtoScopyLabel: Clicked at image position:" << imagePos;
    }
    
    QLabel::mousePressEvent(event);
}

void OtoScopyLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (!_clickEnabled) {
        QLabel::mouseMoveEvent(event);
        return;
    }
    
    const QPoint imagePos = mapToImage(event->pos());
    
    if (imagePos.x() >= 0 && imagePos.y() >= 0) {
        emit mouseMoved(imagePos);
    }
    
    QLabel::mouseMoveEvent(event);
}

void OtoScopyLabel::wheelEvent(QWheelEvent* event)
{
    if (!_clickEnabled) {
        QLabel::wheelEvent(event);
        return;
    }
    
    const QPoint imagePos = mapToImage(event->position().toPoint());
    
    if (imagePos.x() >= 0 && imagePos.y() >= 0) {
        emit wheelMoved(event->angleDelta().y(), imagePos);
    }
    
    QLabel::wheelEvent(event);
}

QRect OtoScopyLabel::getImageRect() const
{
    if (!pixmap()) {
        return QRect();
    }
    
    const QPixmap* pm = pixmap();
    const QRect widgetRect = rect();
    
    // Calculate scaled image size while maintaining aspect ratio
    const double aspectRatio = static_cast<double>(pm->width()) / pm->height();
    const double widgetAspectRatio = static_cast<double>(widgetRect.width()) / widgetRect.height();
    
    int imageWidth, imageHeight;
    
    if (aspectRatio > widgetAspectRatio) {
        // Image is wider than widget - fit to width
        imageWidth = widgetRect.width();
        imageHeight = static_cast<int>(imageWidth / aspectRatio);
    } else {
        // Image is taller than widget - fit to height
        imageHeight = widgetRect.height();
        imageWidth = static_cast<int>(imageHeight * aspectRatio);
    }
    
    // Center the image within the widget
    const int x = (widgetRect.width() - imageWidth) / 2;
    const int y = (widgetRect.height() - imageHeight) / 2;
    
    return QRect(x, y, imageWidth, imageHeight);
}

bool OtoScopyLabel::isPointInImage(const QPoint& widgetPos) const
{
    const QRect imageRect = getImageRect();
    return imageRect.contains(widgetPos);
}

#include "OtoScopyLabel.moc"