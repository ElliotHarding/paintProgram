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

Canvas::~Canvas()
{
    if(m_selectionTool)
        delete m_selectionTool;
}

void Canvas::setCurrentTool(Tool t)
{
    m_tool = t;
}

void Canvas::deleteKeyPressed()
{
    if(m_selectionTool && m_tool == TOOL_SELECT)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        QPainter painter(&m_canvasImage);
        painter.fillRect(m_selectionTool->geometry(), QColor(0,0,0,255));

        //Call to redraw
        update(/*posX, posY, 1, 1*/);
    }
}

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.scale(m_zoomFactor, m_zoomFactor);

    QRect rect = QRect(0, 0, m_canvasImage.width(), m_canvasImage.height());
    painter.drawImage(rect, m_canvasImage, m_canvasImage.rect());

    if(m_tool == TOOL_SELECT && m_selectionTool)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(m_c_selectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry());
        painter.fillRect(m_selectionTool->geometry(), m_c_selectionAreaColor);
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
     //todo ~ check if we can take the mouseLocation init out of the two if statements

    if(m_tool == TOOL_PAINT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        updatePixel(mouseLocation.x(), mouseLocation.y());
    }
    else if(m_tool == TOOL_SELECT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        selectionClick(mouseLocation.x(), mouseLocation.y());
    }

    m_bMouseDown = true;
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
        if(m_tool == TOOL_PAINT)
        {
            updatePixel(mouseLocation.x(), mouseLocation.y());
        }
        else if(m_tool == TOOL_SELECT)
        {
            selectionClick(mouseLocation.x(), mouseLocation.y());
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
    //If start of new selection
    if(m_selectionTool == nullptr || !m_bMouseDown)
    {
        m_selectionTool = new QRubberBand(QRubberBand::Rectangle, this);
        m_selectionToolOrigin = QPoint(clickX,clickY);
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }

    else
    {
        m_selectionTool->setGeometry(
                    QRect(m_selectionToolOrigin, QPoint(clickX,clickY)).normalized()
                    );
    }

    update();

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
