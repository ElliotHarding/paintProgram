#include "canvas.h"
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>

Canvas::Canvas(MainWindow* parent, uint width, uint height) :
    m_pParent(parent),
    QTabWidget()
{
    m_canvasImage = QImage(QSize(width, height), QImage::Format_RGB32);

    setMouseTracking(true);
}

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.scale(m_zoomFactor, m_zoomFactor);

    QRect rect = QRect(0, 0, m_canvasImage.width(), m_canvasImage.height());
    painter.drawImage(rect, m_canvasImage, m_canvasImage.rect());
}

void Canvas::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y() > 0)
    {
        m_zoomFactor += m_cZoomIncrement;
    }
    else if(event->angleDelta().y() < 0)
    {
         m_zoomFactor -= m_cZoomIncrement;
    }

    //Call to redraw
    update();
}

void Canvas::mousePressEvent(QMouseEvent *mouseEvent)
{
    m_bMouseDown = true;
    QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
    updatePixel(mouseLocation.x(), mouseLocation.y());
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    m_bMouseDown = false;
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bMouseDown)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(event);
        updatePixel(mouseLocation.x(), mouseLocation.y());
    }
}

QPoint Canvas::getLocationFromMouseEvent(QMouseEvent *event)
{
    QTransform transform;
    transform.scale(m_zoomFactor, m_zoomFactor);
    return transform.inverted().map(QPoint(event->x(), event->y()));
}

void Canvas::updatePixel(uint posX, uint posY)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    //Check positions are in bounds of vector
    if(posX <= m_canvasImage.width() && posY <= m_canvasImage.height())
    {
        QPainter painter(&m_canvasImage);
        QRect rect = QRect(posX, posY, 1, 1);
        painter.fillRect(rect, m_pParent->getSelectedColor());

        //Call to redraw
        update(/*posX, posY, 1, 1*/);
    }
}
