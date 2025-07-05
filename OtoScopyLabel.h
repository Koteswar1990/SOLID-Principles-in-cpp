#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <QPainter>
#include <QPen>

class OtoScopyLabel : public QLabel
{
    Q_OBJECT

public:
    explicit OtoScopyLabel(QWidget* parent = nullptr);
    ~OtoScopyLabel() override = default;

    // Enable/disable click handling
    void setClickEnabled(bool enabled);
    bool isClickEnabled() const;

    // Map widget coordinates to image coordinates
    QPoint mapToImage(const QPoint& widgetPos) const;
    QPoint mapFromImage(const QPoint& imagePos) const;

    // Get the current scale factor
    double getScaleFactor() const;

    // Get actual image size
    QSize getImageSize() const;

signals:
    // Emitted when the user clicks on the image
    void clickedAt(const QPoint& imagePos);
    
    // Emitted when the mouse moves over the image
    void mouseMoved(const QPoint& imagePos);
    
    // Emitted when the mouse wheel is used
    void wheelMoved(int delta, const QPoint& imagePos);

protected:
    // Mouse event handlers
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    bool _clickEnabled;
    
    // Helper method to calculate image bounds within the widget
    QRect getImageRect() const;
    
    // Helper method to check if a point is within the image area
    bool isPointInImage(const QPoint& widgetPos) const;
};