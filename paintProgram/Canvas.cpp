#include "canvas.h"
#include <QWheelEvent>
#include <QDebug>
#include <QSet>
#include <QFileInfo>
#include <stack>

Canvas::Canvas(MainWindow* parent, QImage image) :
    QTabWidget(),
    m_pParent(parent)
{
    m_canvasImage = image;

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

int Canvas::width()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasImage.width();
}

int Canvas::height()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasImage.height();
}

QString Canvas::getSavePath()
{
    return m_savePath;
}

void Canvas::setSavePath(QString path)
{
    m_savePath = path;
}

void Canvas::updateSettings(int width, int height, QString name)
{
    //Create new image based on new settings
    QImage newImage = QImage(QSize(width, height), QImage::Format_ARGB32);

    m_canvasMutex.lock();

    //Fill new image as transparent
    QPainter painter(&newImage);
    painter.setCompositionMode (QPainter::CompositionMode_Clear);
    painter.fillRect(QRect(0, 0, newImage.width(), newImage.height()), Qt::transparent);

    //Paint old image onto new image
    painter.setCompositionMode (QPainter::CompositionMode_Source);
    painter.drawImage(m_canvasImage.rect(), m_canvasImage);

    painter.end();

    m_canvasImage = newImage;

    m_canvasMutex.unlock();

    if(m_savePath != "")
    {
        QFileInfo info(m_savePath);
        m_savePath = info.path() + "/" + name + "." + info.completeSuffix();
        qDebug() << m_savePath;
    }
}

void Canvas::updateCurrentTool(Tool t)
{
    m_tool = t;

    bool doUpdate = false;

    if(m_tool != TOOL_SELECT)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        //Reset selection rectangle tool        
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

        if(m_tool != TOOL_SPREAD_ON_SIMILAR && m_tool != TOOL_DRAG)
        {
            m_selectedPixels.clear();
        }

        canvasMutexLocker.unlock();

        doUpdate = true;
    }

    if(m_tool != TOOL_DRAG)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        //Dump dragged contents onto m_canvasImage
        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_SourceOver);
        painter.drawImage(QRect(m_dragOffsetX, m_dragOffsetY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);

        recordImageHistory();

        //Reset
        m_clipboardImage = QImage();
        m_previousDragPos = m_c_nullDragPos;
        m_dragOffsetX = 0;
        m_dragOffsetY = 0;

        canvasMutexLocker.unlock();

        doUpdate = true;
    }

    if (doUpdate)
        update();
}

void Canvas::deleteKeyPressed()
{
    if(m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_Clear);

        for(QPoint p : m_selectedPixels)
        {
            painter.fillRect(QRect(p.x(), p.y(), 1, 1), Qt::transparent);
        }

        m_selectedPixels.clear();

        recordImageHistory();

        canvasMutexLocker.unlock();

        update();
    }
}

void Canvas::copyKeysPressed()
{
    m_canvasMutex.lock();
    m_pParent->setCopyBuffer(genClipBoard());
    m_canvasMutex.unlock();

    update();
}

void Canvas::cutKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Copy cut pixels to clipboard
    QImage clipBoard = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);

    QPainter dragPainter(&clipBoard);
    dragPainter.setCompositionMode (QPainter::CompositionMode_Source);
    dragPainter.fillRect(clipBoard.rect(), Qt::transparent);

    QPainter painter(&m_canvasImage);
    painter.setCompositionMode (QPainter::CompositionMode_Clear);

    for(QPoint p : m_selectedPixels)
    {
        dragPainter.fillRect(QRect(p.x(), p.y(), 1, 1), m_canvasImage.pixelColor(p.x(), p.y()));
        painter.fillRect(QRect(p.x(), p.y(), 1, 1), Qt::transparent);
    }    

    m_selectedPixels.clear();
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    canvasMutexLocker.unlock();

    recordImageHistory();

    m_pParent->setCopyBuffer(clipBoard);
    update();
}

void Canvas::pasteKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    m_clipboardImage = m_pParent->getCopyBuffer();

    m_selectedPixels.clear();
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    m_previousDragPos = m_c_nullDragPos;
    m_dragOffsetX = 0;
    m_dragOffsetY = 0;

    for(int x = 0; x < m_clipboardImage.width(); x++)
    {
        for(int y = 0; y < m_clipboardImage.height(); y++)
        {
            if(m_clipboardImage.pixelColor(x,y).alpha() > 0)
                m_selectedPixels.push_back(QPoint(x, y));
        }
    }

    canvasMutexLocker.unlock();

    update();
}

void Canvas::undoPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(m_imageHistoryIndex > 0)
    {
        m_canvasImage = m_imageHistory[size_t(--m_imageHistoryIndex)];

        update();
    }
}

void Canvas::redoPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(m_imageHistoryIndex < int(m_imageHistory.size() - 1))
    {
        m_canvasImage = m_imageHistory[size_t(++m_imageHistoryIndex)];

        update();
    }
}

QImage Canvas::getImageCopy()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasImage;
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
    m_canvasMutex.lock();

    //Setup painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    //Zoom painter
    painter.scale(m_zoomFactor, m_zoomFactor);

    //Switch out transparent pixels for grey-white pattern
    drawTransparentPixels(painter, m_panOffsetX, m_panOffsetY);

    //Draw current image

    QRect rect = QRect(m_panOffsetX, m_panOffsetY, m_canvasImage.width(), m_canvasImage.height());
    painter.drawImage(rect, m_canvasImage, m_canvasImage.rect());

    //Draw dragging pixels
    painter.drawImage(QRect(m_dragOffsetX + m_panOffsetX, m_dragOffsetY + m_panOffsetY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);

    //Draw highlighed pixels
    for(QPoint p : m_selectedPixels)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.fillRect(QRect(p.x() + m_panOffsetX, p.y() + m_panOffsetY, 1, 1), m_c_selectionAreaColor);
    }

    //Draw selection tool
    if(m_tool == TOOL_SELECT)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(m_c_selectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry().translated(m_panOffsetX, m_panOffsetY));
    }

    m_canvasMutex.unlock();
}

//Called when m_canvasMutex is locked
void Canvas::drawTransparentPixels(QPainter& painter, float offsetX, float offsetY)
{
    for(int x = 0; x < m_canvasImage.width(); x++)
    {
        for(int y = 0; y < m_canvasImage.height(); y++)
        {
            if(m_canvasImage.pixelColor(x,y).alpha() < 255)
            {
                const QColor col = (x % 2 == 0) ?
                            (y % 2 == 0) ? m_c_transparentWhite : m_c_transparentGrey
                                         :
                            (y % 2 == 0) ? m_c_transparentGrey : m_c_transparentWhite;

                painter.fillRect(QRect(x + offsetX, y + offsetY, 1, 1), col);
            }
        }
    }
}

void Canvas::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y() > 0)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);
        m_zoomFactor += m_cZoomIncrement;
    }
    else if(event->angleDelta().y() < 0)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);
         m_zoomFactor -= m_cZoomIncrement;
    }

    //Call to redraw
    update();
}

void Canvas::mousePressEvent(QMouseEvent *mouseEvent)
{
    m_bMouseDown = true;

    //todo ~ check if we can take the mouseLocation init out of the if statements

    if(m_tool == TOOL_PAINT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        paintBrush(mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor());
    }
    else if(m_tool == TOOL_ERASER)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
        paintBrush(mouseLocation.x(), mouseLocation.y(), Qt::transparent);
    }
    else if(m_tool == TOOL_SELECT)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);

        QMutexLocker canvasMutexLocker(&m_canvasMutex);
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
    else if(m_tool == TOOL_BUCKET)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);

        QMutexLocker canvasMutexLocker(&m_canvasMutex);
        floodFillOnSimilar(m_canvasImage, m_pParent->getSelectedColor(), mouseLocation.x(), mouseLocation.y(), m_pParent->getSpreadSensitivity());

        update();
    }
    else if(m_tool == TOOL_COLOR_PICKER)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);

        QMutexLocker canvasMutexLocker(&m_canvasMutex);
        m_pParent->setSelectedColor(m_canvasImage.pixelColor(mouseLocation.x(), mouseLocation.y()));
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    m_bMouseDown = false;

    if(m_tool == TOOL_SELECT)
    {
        releaseSelect();
    }
    else if(m_tool == TOOL_PAN)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);
        m_previousPanPos = m_c_nullPanPos;
    }
    else if (m_tool == TOOL_PAINT || m_tool == TOOL_ERASER)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);
        recordImageHistory();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bMouseDown)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(event);
        if(m_tool == TOOL_PAINT)
        {
            paintBrush(mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor());
        }
        else if(m_tool == TOOL_ERASER)
        {
            paintBrush(mouseLocation.x(), mouseLocation.y(), Qt::transparent);
        }
        else if(m_tool == TOOL_SELECT)
        {
            m_canvasMutex.lock();
            m_selectionTool->setGeometry(
                        QRect(m_selectionToolOrigin, QPoint(mouseLocation.x(), mouseLocation.y())).normalized()
                        );
            m_canvasMutex.unlock();
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
                m_canvasMutex.lock();
                m_panOffsetX += mouseLocation.x() - m_previousPanPos.x();
                m_panOffsetY += mouseLocation.y() - m_previousPanPos.y();

                m_previousPanPos = mouseLocation;

                m_canvasMutex.unlock();

                update();                
            }
        }
        else if(m_tool == TOOL_DRAG)
        {
            dragPixels(mouseLocation);
        }
    }
}

void Canvas::releaseSelect()
{
    m_canvasMutex.lock();

    //Prep vector container of highlighted pixels
    std::vector<std::vector<bool>> highlightedPixels(m_canvasImage.width(), std::vector<bool>(m_canvasImage.height(), false));
    {
        for(QPoint selectedPixel : m_selectedPixels)
        {
            highlightedPixels[selectedPixel.x()][selectedPixel.y()] = true;
        }
    }

    const QRect geometry = m_selectionTool->geometry();
    m_canvasMutex.unlock();
    for (int x = geometry.x(); x < geometry.x() + geometry.width(); x++)
    {
        for (int y = geometry.y(); y < geometry.y() + geometry.height(); y++)
        {
            highlightedPixels[x][y] = true;
        }
    }

    m_canvasMutex.lock();

    //Return highlighted pixels from algorithm to m_selectedPixels
    m_selectedPixels.clear();
    for(int x = 0; x < highlightedPixels.size(); x++)
    {
        for(int y = 0; y < highlightedPixels[x].size(); y++)
        {
            if(highlightedPixels[x][y])
                m_selectedPixels.push_back(QPoint(x,y));
        }
    }

    m_canvasMutex.unlock();

    update();
}

QPoint Canvas::getLocationFromMouseEvent(QMouseEvent *event)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    QTransform transform;
    transform.scale(m_zoomFactor, m_zoomFactor);
    const QPoint zoomPoint = transform.inverted().map(QPoint(event->x(), event->y()));
    return QPoint(zoomPoint.x() + m_panOffsetX * -1, zoomPoint.y() + m_panOffsetY * -1);
}

/*
void spreadSelectRecursive(QImage& image, QList<QPoint>& selectedPixels, QColor colorToSpreadOver, int sensitivty, int x, int y)
{
    if(x < image.width() && x > -1 && y < image.height() && y > -1)
    {
        const QColor pixelColor = QColor(image.pixel(x,y));
        if(pixelColor.red() <= colorToSpreadOver.red() + sensitivty && pixelColor.red() >= colorToSpreadOver.red() - sensitivty &&
            pixelColor.green() <= colorToSpreadOver.green() + sensitivty && pixelColor.green() >= colorToSpreadOver.green() - sensitivty &&
                pixelColor.blue() <= colorToSpreadOver.blue() + sensitivty && pixelColor.blue() >= colorToSpreadOver.blue() - sensitivty
                )
        {
            if(selectedPixels.indexOf(QPoint(x,y)) == -1)
            {
                selectedPixels.push_back(QPoint(x,y));

                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, sensitivty, x + 1, y);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, sensitivty, x - 1, y);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, sensitivty, x, y + 1);
                spreadSelectRecursive(image, selectedPixels, colorToSpreadOver, sensitivty, x, y - 1);
            }
        }
    }
}*/

void spreadSelectFunction(QImage& image, std::vector<std::vector<bool>>& selectedPixels, QColor colorToSpreadOver, int sensitivty, int startX, int startY)
{
    if(startX < image.width() && startX > -1 && startY < image.height() && startY > -1)
    {
        std::stack<QPoint> stack;
        stack.push(QPoint(startX,startY));

        while (stack.size() > 0)
        {
            QPoint p = stack.top();
            stack.pop();
            const int x = p.x();
            const int y = p.y();
            if (y < 0 || y > image.height() || x < 0 || x > image.width())
                continue;

            const QColor pixelColor = image.pixelColor(x,y);
            if (pixelColor.red() <= colorToSpreadOver.red() + sensitivty && pixelColor.red() >= colorToSpreadOver.red() - sensitivty &&
                pixelColor.green() <= colorToSpreadOver.green() + sensitivty && pixelColor.green() >= colorToSpreadOver.green() - sensitivty &&
                pixelColor.blue() <= colorToSpreadOver.blue() + sensitivty && pixelColor.blue() >= colorToSpreadOver.blue() - sensitivty &&
                selectedPixels[x][y] == false)
            {
                selectedPixels[x][y] = true;
                stack.push(QPoint(x + 1, y));
                stack.push(QPoint(x - 1, y));
                stack.push(QPoint(x, y + 1));
                stack.push(QPoint(x, y - 1));
            }
        }
    }
}

void Canvas::spreadSelectArea(int x, int y)
{
    m_canvasMutex.lock();

    //Prep vector container of highlighted pixels for spread algorithm
    std::vector<std::vector<bool>> highlightedPixels(m_canvasImage.width(), std::vector<bool>(m_canvasImage.height(), false));
    if(!m_pParent->isCtrlPressed())
    {
        m_selectedPixels.clear();
    }
    else
    {
        for(QPoint selectedPixel : m_selectedPixels)
        {
            highlightedPixels[selectedPixel.x()][selectedPixel.y()] = true;
        }
    }

    //Do spread highlight
    spreadSelectFunction(m_canvasImage, highlightedPixels, m_canvasImage.pixel(x,y), m_pParent->getSpreadSensitivity(), x, y);

    //Return highlighted pixels from algorithm to m_selectedPixels
    m_selectedPixels.clear();
    for(int x = 0; x < highlightedPixels.size(); x++)
    {
        for(int y = 0; y < highlightedPixels[x].size(); y++)
        {
            if(highlightedPixels[x][y])
                m_selectedPixels.push_back(QPoint(x,y));
        }
    }

    m_canvasMutex.unlock();

    //Call to redraw
    update();
}

void Canvas::floodFillOnSimilar(QImage &image, QColor newColor, int startX, int startY, int sensitivity)
{
    if(startX < image.width() && startX > -1 && startY < image.height() && startY > -1)
    {
        const QColor originalPixelColor = QColor(image.pixel(startX, startY));

        QPainter painter(&image);
        painter.setCompositionMode (QPainter::CompositionMode_Source);

        std::stack<QPoint> stack;
        stack.push(QPoint(startX,startY));

        while (stack.size() > 0)
        {
            QPoint p = stack.top();
            stack.pop();
            const int x = p.x();
            const int y = p.y();
            if (y < 0 || y >= image.height() || x < 0 || x >= image.width())
                continue;

            const QColor pixelColor = image.pixelColor(x,y);
            if (
                //Check pixel color in sensitivity range
                pixelColor.red() <= originalPixelColor.red() + sensitivity && pixelColor.red() >= originalPixelColor.red() - sensitivity &&
                pixelColor.green() <= originalPixelColor.green() + sensitivity && pixelColor.green() >= originalPixelColor.green() - sensitivity &&
                pixelColor.blue() <= originalPixelColor.blue() + sensitivity && pixelColor.blue() >= originalPixelColor.blue() - sensitivity &&

                //Check not excat same color
                pixelColor != newColor
                    )
            {
                //Switch color
                QRect rect = QRect(x, y, 1, 1);
                painter.fillRect(rect, newColor);

                stack.push(QPoint(x + 1, y));
                stack.push(QPoint(x - 1, y));
                stack.push(QPoint(x, y + 1));
                stack.push(QPoint(x, y - 1));
            }
        }
    }
}

//Requires m_canvasMutex to be locked
QImage Canvas::genClipBoard()
{
    //Prep selected pixels for dragging
    QImage clipboard = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);
    QPainter dragPainter(&clipboard);
    dragPainter.setCompositionMode (QPainter::CompositionMode_Clear);
    dragPainter.fillRect(clipboard.rect(), Qt::transparent);

    dragPainter.setCompositionMode (QPainter::CompositionMode_Source);
    for(QPoint p : m_selectedPixels)
    {
        dragPainter.fillRect(QRect(p.x(), p.y(), 1, 1), m_canvasImage.pixelColor(p.x(), p.y()));
    }

    return clipboard;
}

void Canvas::dragPixels(QPoint mouseLocation)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //If starting dragging
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
            if(m_clipboardImage == QImage())
                m_clipboardImage = genClipBoard();

            m_previousDragPos = mouseLocation;
            m_dragOffsetX = 0;
            m_dragOffsetY = 0;
        }
    }
    else //If currently dragging
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

void Canvas::paintBrush(uint posX, uint posY, QColor col)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

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
