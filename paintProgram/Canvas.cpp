#include "canvas.h"
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>
#include <QSet>

Canvas::Canvas(MainWindow* parent, uint width, uint height) :
    QTabWidget(),
    m_pParent(parent)
{
    m_canvasImage = QImage(QSize(width, height), QImage::Format_RGB32);

    m_selectionTool = new QRubberBand(QRubberBand::Rectangle, this);
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    setMouseTracking(true);
}

Canvas::~Canvas()
{
    if(m_selectionTool)
        delete m_selectionTool;
}

void Canvas::setCurrentTool(Tool t)
{
    m_tool = t;

    std::lock_guard<std::mutex> lock(m_canvasMutex);
    m_selectedPixels.clear();

    if(m_tool != TOOL_SELECT)
    {
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }

    update();
}

void Canvas::deleteKeyPressed()
{
    if(m_tool == TOOL_SELECT)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        painter.fillRect(m_selectionTool->geometry(), QColor(0,0,0,0));

        //Call to redraw
        update();
    }
}

void Canvas::copyKeysPressed()
{

}

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.scale(m_zoomFactor, m_zoomFactor);

    QRect rect = QRect(0, 0, m_canvasImage.width(), m_canvasImage.height());
    painter.drawImage(rect, m_canvasImage, m_canvasImage.rect());

    for(QPoint p : m_selectedPixels)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.fillRect(QRect(p.x(), p.y(), 1, 1), m_c_selectionAreaColor);
    }

    if(m_tool == TOOL_SELECT)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(m_c_selectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry());
    }
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

    //todo ~ check if we can take the mouseLocation init out of the two if statements

    if(m_tool == TOOL_PAINT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        paintPixel(mouseLocation.x(), mouseLocation.y());
    }
    else if(m_tool == TOOL_SELECT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);

        std::lock_guard<std::mutex> lock(m_canvasMutex);
        m_selectionToolOrigin = QPoint(mouseLocation.x(), mouseLocation.y());
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }
    else if(m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        spreadSelectArea(mouseLocation.x(), mouseLocation.y());
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    m_bMouseDown = false;

    if(m_tool == TOOL_SELECT)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        const QRect geometry = m_selectionTool->geometry();
        for (int x = geometry.x(); x < geometry.x() + geometry.width(); x++)
        {
            for (int y = geometry.y(); y < geometry.y() + geometry.height(); y++)
            {
                if(m_selectedPixels.indexOf(QPoint(x,y)) == -1)
                {
                    m_selectedPixels.push_back(QPoint(x,y));
                }
            }
        }

        update();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bMouseDown)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(event);
        if(m_tool == TOOL_PAINT)
        {
            paintPixel(mouseLocation.x(), mouseLocation.y());
        }
        else if(m_tool == TOOL_SELECT)
        {
            std::lock_guard<std::mutex> lock(m_canvasMutex);
            m_selectionTool->setGeometry(
                        QRect(m_selectionToolOrigin, QPoint(mouseLocation.x(), mouseLocation.y())).normalized()
                        );
            update();
        }
    }
}

QPoint Canvas::getLocationFromMouseEvent(QMouseEvent *event)
{
    QTransform transform;
    transform.scale(m_zoomFactor, m_zoomFactor);
    return transform.inverted().map(QPoint(event->x(), event->y()));
}

void Canvas::selectionClick(int clickX, int clickY)
{

}

void spreadSelectRecursive(QImage image, QList<QPoint>& selectedPixels, QColor colorToSpreadOver, int x, int y)
{
    if(x <= image.width() && x > -1 && y <= image.height() && y > -1)
    {
        if(QColor(image.pixel(x,y)) == colorToSpreadOver)
        {
            if(selectedPixels.indexOf(QPoint(x,y)) == -1)
            {
                selectedPixels.push_back(QPoint(x,y));
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, x + 1, y);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, x - 1, y);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, x, y + 1);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, x, y - 1);
            }
        }
    }
}

void Canvas::spreadSelectArea(int x, int y)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    m_selectedPixels.clear();

    if(x <= m_canvasImage.width() && y <= m_canvasImage.height())
    {
        QColor initalPixel = m_canvasImage.pixel(x,y);
        spreadSelectRecursive(m_canvasImage, m_selectedPixels, initalPixel, x, y);

        //Call to redraw
        update();
    }
}

void Canvas::paintPixel(uint posX, uint posY)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    //Check positions are in bounds of vector
    if(posX <= m_canvasImage.width() && posY <= m_canvasImage.height())
    {
        QPainter painter(&m_canvasImage);
        QRect rect = QRect(posX, posY, 1, 1);
        painter.fillRect(rect, m_pParent->getSelectedColor());

        //Call to redraw
        update();
    }
}
