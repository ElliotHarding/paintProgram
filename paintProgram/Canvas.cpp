#include "canvas.h"
#include <QWheelEvent>
#include <QDebug>
#include <QSet>

Canvas::Canvas(MainWindow* parent, uint width, uint height) :
    QTabWidget(),
    m_pParent(parent)
{
    m_canvasImage = QImage(QSize(width, height), QImage::Format_ARGB32);

    QPainter painter(&m_canvasImage);
    painter.setCompositionMode (QPainter::CompositionMode_Clear);
    painter.fillRect(m_canvasImage.rect(), Qt::transparent);

    recordImageHistory();

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

    if(m_tool != TOOL_SELECT)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

        if(m_tool != TOOL_SPREAD_ON_SIMILAR && m_tool != TOOL_DRAG)
        {
            m_selectedPixels.clear();
        }
    }

    update();
}

void Canvas::deleteKeyPressed()
{
    if(m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_Clear);

        for(QPoint p : m_selectedPixels)
        {
            painter.fillRect(QRect(p.x(), p.y(), 1, 1), Qt::transparent);
        }

        m_selectedPixels.clear();

        recordImageHistory();

        update();
    }
}

void Canvas::copyKeysPressed()
{
    if(m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        m_copyBuffer.clear();

        for(QPoint p : m_selectedPixels)
        {
            m_copyBuffer.push_back({p, m_canvasImage.pixelColor(p.x(), p.y())});
        }

        update();
    }
}

void Canvas::pasteKeysPressed()
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    QPainter painter(&m_canvasImage);
    painter.setCompositionMode (QPainter::CompositionMode_Source);

    m_selectedPixels.clear();

    for(CopyPixel pixel : m_copyBuffer)
    {
        painter.fillRect(QRect(pixel.position.x(), pixel.position.y(), 1, 1), pixel.color);
        m_selectedPixels.push_back(QPoint(pixel.position.x(), pixel.position.y()));
    }

    update();
}

void Canvas::undoPressed()
{
    if(m_imageHistoryIndex > 0)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        m_canvasImage = m_imageHistory[size_t(--m_imageHistoryIndex)];

        update();
    }
}

void Canvas::redoPressed()
{
    if(m_imageHistoryIndex < int(m_imageHistory.size() - 1))
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        m_canvasImage = m_imageHistory[size_t(++m_imageHistoryIndex)];

        update();
    }
}

//Function called when m_canvasMutex is locked
void Canvas::recordImageHistory()
{
    m_imageHistory.push_back(m_canvasImage);

    if(m_c_maxHistory < m_imageHistory.size())
    {
        m_imageHistory.erase(m_imageHistory.begin());
    }

    m_imageHistoryIndex = m_imageHistory.size() - 1;
}

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);
    std::lock_guard<std::mutex> panOffsetLock(m_panOffsetMutex);

    //Setup painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    //Zoom painter
    painter.scale(m_zoomFactor, m_zoomFactor);

    //Draw current image
    QRect rect = QRect(m_panOffsetX, m_panOffsetY, m_canvasImage.width(), m_canvasImage.height());
    painter.drawImage(rect, m_canvasImage, m_canvasImage.rect());

    //Switch out transparent pixels for grey-white pattern
    drawTransparentPixels(painter, m_panOffsetX, m_panOffsetY);

    //Draw dragging pixels
    painter.drawImage(QRect(m_dragOffsetX, m_dragOffsetY, m_draggingPixelsImage.width(), m_draggingPixelsImage.height()), m_draggingPixelsImage);

    //Draw highlighed pixels
    for(QPoint p : m_selectedPixels)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.fillRect(QRect(p.x(), p.y(), 1, 1), m_c_selectionAreaColor);
    }

    //Draw selection tool
    if(m_tool == TOOL_SELECT)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(m_c_selectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry());
    }
}

void Canvas::drawTransparentPixels(QPainter& painter, float offsetX, float offsetY)
{
    for(int x = 0; x < m_canvasImage.width(); x++)
    {
        for(int y = 0; y < m_canvasImage.height(); y++)
        {
            if(m_canvasImage.pixelColor(x,y) == Qt::transparent)
            {
                QColor col;

                if(x % 2 == 0)
                {
                    if(y % 2 == 0)
                    {
                        col = QColor(255,255,255,255);
                    }
                    else
                    {
                        col = QColor(190,190,190,255);
                    }
                }
                else
                {
                    if(y % 2 == 0)
                    {
                        col = QColor(190,190,190,255);
                    }
                    else
                    {
                        col = QColor(255,255,255,255);
                    }
                }

                painter.fillRect(QRect(x + offsetX, y + offsetY, 1, 1), col);
            }
        }
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
        paintPixel(mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor());
    }
    else if(m_tool == TOOL_ERASER)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        paintPixel(mouseLocation.x(), mouseLocation.y(), Qt::transparent);
    }
    else if(m_tool == TOOL_SELECT)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);

        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);

        if(!m_pParent->isCtrlPressed())
        {
            m_selectedPixels.clear();
        }

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
    else if(m_tool == TOOL_PAN)
    {
        m_previousPanPos = m_c_nullPanPos;
    }
    else if(m_tool == TOOL_DRAG)
    {
        m_previousDragPos = m_c_nullDragPos;
    }
    else if (m_tool == TOOL_PAINT || m_tool == TOOL_ERASER)
    {
        std::lock_guard<std::mutex> lock(m_canvasMutex);
        recordImageHistory();
    }
    else if(m_tool == TOOL_DRAG)
    {
        //todo : fix this, it dosent need to be this compliated
        //m_draggingPixelsImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);
        QPainter painter(&m_draggingPixelsImage);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        painter.fillRect(m_draggingPixelsImage.rect(), Qt::transparent);

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
            paintPixel(mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor());
        }
        else if(m_tool == TOOL_ERASER)
        {
            paintPixel(mouseLocation.x(), mouseLocation.y(), Qt::transparent);
        }
        else if(m_tool == TOOL_SELECT)
        {
            std::lock_guard<std::mutex> lock(m_canvasMutex);
            m_selectionTool->setGeometry(
                        QRect(m_selectionToolOrigin, QPoint(mouseLocation.x(), mouseLocation.y())).normalized()
                        );
            update();
        }
        else if(m_tool == TOOL_PAN)
        {
            if(m_previousPanPos == m_c_nullPanPos)
            {
                m_previousPanPos = mouseLocation;
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_panOffsetMutex);
                m_panOffsetX += m_c_panSpeed * (mouseLocation.x() - m_previousPanPos.x());
                m_panOffsetY += m_c_panSpeed * (mouseLocation.y() - m_previousPanPos.y());

                update();

                m_previousPanPos = mouseLocation;
            }
        }
        else if(m_tool == TOOL_DRAG)
        {
            if(m_previousDragPos == m_c_nullDragPos)
            {
                //check if mouse is over selection area
                bool draggingSelected = false;
                for(QPoint p : m_selectedPixels)
                {
                    if(p.x() == mouseLocation.x() && p.y() == mouseLocation.y())
                    {
                        draggingSelected = true;
                        break;
                    }
                }

                if(draggingSelected)
                {
                    m_previousDragPos = mouseLocation;

                    m_draggingPixelsImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);
                    QPainter painter(&m_draggingPixelsImage);
                    painter.setCompositionMode (QPainter::CompositionMode_Source);
                    painter.fillRect(m_draggingPixelsImage.rect(), Qt::transparent);

                    for(QPoint p : m_selectedPixels)
                    {
                        painter.fillRect(QRect(p.x(), p.y(), 1, 1), m_canvasImage.pixelColor(p.x(), p.y()));
                    }
                }
            }
            else
            {
                const int offsetX = (mouseLocation.x() - m_previousDragPos.x());
                const int offsetY = (mouseLocation.y() - m_previousDragPos.y());

                m_dragOffsetX += offsetX;
                m_dragOffsetY += offsetY;

                for(QPoint& p : m_selectedPixels)
                {
                    p.setX(p.x() + offsetX);
                    p.setY(p.y() + offsetY);
                }

                update();

                m_previousDragPos = mouseLocation;
            }
        }
    }
}

QPoint Canvas::getLocationFromMouseEvent(QMouseEvent *event)
{
    QTransform transform;
    transform.scale(m_zoomFactor, m_zoomFactor);
    return transform.inverted().map(QPoint(event->x(), event->y()));
}

void spreadSelectRecursive(QImage image, QList<QPoint>& selectedPixels, QColor colorToSpreadOver, int x, int y)
{
    if(x < image.width() && x > -1 && y < image.height() && y > -1)
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

    if(!m_pParent->isCtrlPressed())
    {
        m_selectedPixels.clear();
    }

    if(x <= m_canvasImage.width() && y <= m_canvasImage.height())
    {
        QColor initalPixel = m_canvasImage.pixel(x,y);
        spreadSelectRecursive(m_canvasImage, m_selectedPixels, initalPixel, x, y);

        //Call to redraw
        update();
    }
}

void Canvas::paintPixel(uint posX, uint posY, QColor col)
{
    std::lock_guard<std::mutex> lock(m_canvasMutex);

    //Check positions are in bounds of vector
    if(posX <= m_canvasImage.width() && posY <= m_canvasImage.height())
    {
        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        QRect rect = QRect(posX - m_pParent->getBrushSize()/2, posY - m_pParent->getBrushSize()/2, m_pParent->getBrushSize(), m_pParent->getBrushSize());
        painter.fillRect(rect, col);

        //Call to redraw
        update();
    }
}
