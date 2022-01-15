#include "canvas.h"
#include <QWheelEvent>
#include <QDebug>
#include <QSet>
#include <QFileInfo>
#include <stack>
#include <QPainterPath>

namespace Constants
{
const QPoint NullDragPoint = QPoint(0,0);
const QColor ImageBorderColor = QColor(200,200,200,255);
}

Canvas::Canvas(MainWindow* parent, QImage image) :
    QTabWidget(),
    m_previousDragPos(Constants::NullDragPoint),
    m_pParent(parent)
{
    m_canvasImage = image;

    m_selectionTool = new QRubberBand(QRubberBand::Rectangle, this);
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    setMouseTracking(true);
}

Canvas::~Canvas()
{
    if(m_selectionTool)
        delete m_selectionTool;
}

void Canvas::addedToTab()
{
    updateCenter();

    const float x = float(geometry().width()) / float(m_canvasImage.width());
    const float y = float(geometry().height()) / float(m_canvasImage.height());
    m_zoomFactor = x < y ? x : y;
    if(m_zoomFactor < float(0.1))
        m_zoomFactor = 0.1;

    m_panOffsetX = m_center.x() - (m_canvasImage.width() / 2);
    m_panOffsetY = m_center.y() - (m_canvasImage.height() / 2);

    m_textDrawLocation = QPoint(m_canvasImage.width() / 2, m_canvasImage.height() / 2);

    recordImageHistory();
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

void Canvas::updateText(QFont font)
{
    writeText("", font);
}

void Canvas::writeText(QString letter, QFont font)
{
    if(m_tool == TOOL_TEXT)
    {
        m_textToDraw += letter;

        m_canvasMutex.lock();

        m_clipboardImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);

        QPainter textPainter(&m_clipboardImage);
        textPainter.setCompositionMode (QPainter::CompositionMode_Clear);
        textPainter.fillRect(m_clipboardImage.rect(), Qt::transparent);

        textPainter.setCompositionMode (QPainter::CompositionMode_Source);
        textPainter.setPen(m_pParent->getSelectedColor());
        textPainter.setFont(font);

        textPainter.drawText(m_textDrawLocation, m_textToDraw);

        m_canvasMutex.unlock();

        update();
    }
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
    if(m_tool == TOOL_TEXT && t != TOOL_TEXT)
        m_textToDraw = "";

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
        m_previousDragPos = Constants::NullDragPoint;
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

QImage generateClipBoard(QImage& canvas, QList<QPoint>& selectedPixels)
{
    //Prep selected pixels for dragging
    QImage clipboard = QImage(QSize(canvas.width(), canvas.height()), QImage::Format_ARGB32);
    QPainter dragPainter(&clipboard);
    dragPainter.setCompositionMode (QPainter::CompositionMode_Clear);
    dragPainter.fillRect(clipboard.rect(), Qt::transparent);

    dragPainter.setCompositionMode (QPainter::CompositionMode_Source);
    for(QPoint p : selectedPixels)
    {
        dragPainter.fillRect(QRect(p.x(), p.y(), 1, 1), canvas.pixelColor(p));
    }

    return clipboard;
}

void Canvas::copyKeysPressed()
{   
    m_canvasMutex.lock();
    m_pParent->setCopyBuffer(generateClipBoard(m_canvasImage, m_selectedPixels));
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

    m_previousDragPos = Constants::NullDragPoint;
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

void Canvas::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);

    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    updateCenter();

    m_panOffsetX = m_center.x() - (m_canvasImage.width() / 2);
    m_panOffsetY = m_center.y() - (m_canvasImage.height() / 2);

    if(m_textDrawLocation.x() > m_canvasImage.width() || m_textDrawLocation.y() > m_canvasImage.height())
        m_textDrawLocation = QPoint(m_canvasImage.width() / 2, m_canvasImage.height() / 2);

    update();
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
    painter.translate(m_center);
    painter.scale(m_zoomFactor, m_zoomFactor);
    painter.translate(-m_center);

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

    painter.setPen(QPen(Constants::ImageBorderColor, 1/m_zoomFactor));
    painter.drawRect(m_canvasImage.rect().translated(m_panOffsetX, m_panOffsetY));

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
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    const int direction = event->angleDelta().y() > 0 ? 1 : -1;

    const int xFromCenter = event->x() - m_center.x();
    m_panOffsetX -= xFromCenter * 0.05 * direction;

    const int yFromCenter = event->y() - m_center.y();
    m_panOffsetY -= yFromCenter * 0.05 * direction;

    if(event->angleDelta().y() > 0)
    {
        m_zoomFactor += m_cZoomIncrement;
    }
    else if(event->angleDelta().y() < 0)
    {
        if(m_zoomFactor - m_cZoomIncrement > 0)
            m_zoomFactor -= m_cZoomIncrement;
    }

    //Call to redraw
    update();
}

QPoint getPositionRelativeCenterdAndZoomedCanvas(QPoint globalPos, QPoint& center, float& zoomFactor, float& offsetX, float& offsetY)
{
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.scale(zoomFactor, zoomFactor);
    transform.translate(-center.x(), -center.y());
    const QPoint zoomPoint = transform.inverted().map(QPoint(globalPos.x(), globalPos.y()));
    return QPoint(zoomPoint.x() - offsetX, zoomPoint.y() - offsetY);
}

void paintRect(QImage& canvas, const uint x, const uint y, const QColor col, const uint widthHeight)
{
    if(x <= canvas.width() && y <= canvas.height())
    {
        QPainter painter(&canvas);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        QRect rect = QRect(x - widthHeight/2, y - widthHeight/2, widthHeight, widthHeight);
        painter.fillRect(rect, col);
    }
}

void spreadSelectSimilarColorFunction(QImage& image, std::vector<std::vector<bool>>& selectedPixels, QPoint startPixel, int sensitivty)
{
    const QColor colorToSpreadOver = image.pixelColor(startPixel);

    std::stack<QPoint> stack;
    stack.push(startPixel);

    while (stack.size() > 0)
    {
        QPoint p = stack.top();
        stack.pop();
        const int x = p.x();
        const int y = p.y();
        if (y < 0 || y >= image.height() || x < 0 || x >= image.width())
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

void spreadSelectSimilarColor(QImage& canvas, QList<QPoint>& selectedPixels, const QPoint startPixel, const int spreadSensitivity)
{
    if(startPixel.x() <= canvas.width() && startPixel.x() > -1 && startPixel.y() <= canvas.height() && startPixel.y() > -1)
    {
        //Prep vector container of highlighted pixels for spread algorithm
        std::vector<std::vector<bool>> highlightedPixels(canvas.width(), std::vector<bool>(canvas.height(), false));
        for(QPoint selectedPixel : selectedPixels)
        {
            highlightedPixels[selectedPixel.x()][selectedPixel.y()] = true;
        }

        //Do spread highlight
        spreadSelectSimilarColorFunction(canvas, highlightedPixels, startPixel, spreadSensitivity);

        //Return highlighted pixels from algorithm to selectedPixels
        selectedPixels.clear();
        for(int x = 0; x < highlightedPixels.size(); x++)
        {
            for(int y = 0; y < highlightedPixels[x].size(); y++)
            {
                if(highlightedPixels[x][y])
                    selectedPixels.push_back(QPoint(x,y));
            }
        }
    }
}

void floodFillOnSimilar(QImage &image, QColor newColor, int startX, int startY, int sensitivity)
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

void Canvas::mousePressEvent(QMouseEvent *mouseEvent)
{
    m_bMouseDown = true;

    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(mouseEvent->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);

    if(m_tool == TOOL_PAINT)
    {
        paintRect(m_canvasImage, mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor(), m_pParent->getBrushSize());
        update();
    }
    else if(m_tool == TOOL_ERASER)
    {
        paintRect(m_canvasImage, mouseLocation.x(), mouseLocation.y(), Qt::transparent, m_pParent->getBrushSize());
        update();
    }
    else if(m_tool == TOOL_SELECT)
    {
        if(!m_pParent->isCtrlPressed())
        {
            m_selectedPixels.clear();
        }

        m_selectionToolOrigin = mouseLocation;
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }
    else if(m_tool == TOOL_SPREAD_ON_SIMILAR)
    {        
        if(!m_pParent->isCtrlPressed())
        {
            m_selectedPixels.clear();
        }

        spreadSelectSimilarColor(m_canvasImage, m_selectedPixels, mouseLocation, m_pParent->getSpreadSensitivity());

        update();
    }
    else if(m_tool == TOOL_BUCKET)
    {
        floodFillOnSimilar(m_canvasImage, m_pParent->getSelectedColor(), mouseLocation.x(), mouseLocation.y(), m_pParent->getSpreadSensitivity());

        update();
    }
    else if(m_tool == TOOL_COLOR_PICKER)
    {
        m_pParent->setSelectedColor(m_canvasImage.pixelColor(mouseLocation.x(), mouseLocation.y()));
    }
    else if(m_tool == TOOL_TEXT)
    {
        m_textDrawLocation = mouseLocation;

        canvasMutexLocker.unlock();

        updateText(m_pParent->getTextFont());

        canvasMutexLocker.relock();
    }
    else if(m_tool == TOOL_SHAPE)
    {
        m_clipboardImage == QImage();
        m_dragOffsetX = 0;
        m_dragOffsetY = 0;
        m_drawShapeOrigin = mouseLocation;
    }
}

QList<QPoint> combineSelections(QImage& image, QList<QPoint> originalSelectedPixels, QRubberBand* newSelectionArea)
{
    if(newSelectionArea == nullptr)
        return QList<QPoint>();

    //Prep vector container of highlighted pixels
    std::vector<std::vector<bool>> highlightedPixels(image.width(), std::vector<bool>(image.height(), false));
    {
        for(QPoint selectedPixel : originalSelectedPixels)
        {
            highlightedPixels[selectedPixel.x()][selectedPixel.y()] = true;
        }
    }

    const QRect geometry = newSelectionArea->geometry();
    for (int x = geometry.x(); x < geometry.x() + geometry.width(); x++)
    {
        for (int y = geometry.y(); y < geometry.y() + geometry.height(); y++)
        {
            if(y > -1 && y < highlightedPixels[x].size() && x > -1 && y < highlightedPixels.size())
                highlightedPixels[x][y] = true;
        }
    }

    //Return highlighted pixels from algorithm to m_selectedPixels
    QList<QPoint> newleySelectedPixels;
    for(int x = 0; x < highlightedPixels.size(); x++)
    {
        for(int y = 0; y < highlightedPixels[x].size(); y++)
        {
            if(highlightedPixels[x][y])
                newleySelectedPixels.push_back(QPoint(x,y));
        }
    }

    return newleySelectedPixels;
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_bMouseDown = false;

    if(m_tool == TOOL_SELECT)
    {
        m_selectedPixels = combineSelections(m_canvasImage, m_selectedPixels, m_selectionTool);
        update();
    }
    else if(m_tool == TOOL_PAN)
    {
        m_previousPanPos = m_c_nullPanPos;
    }
    else if (m_tool == TOOL_PAINT || m_tool == TOOL_ERASER)
    {
        recordImageHistory();
    }
    else if(m_tool == TOOL_SHAPE)
    {
        //Dump dragged contents onto m_canvasImage
        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_SourceOver);
        painter.drawImage(QRect(m_dragOffsetX, m_dragOffsetY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);

        recordImageHistory();

        //Reset
        m_clipboardImage = QImage();
        m_previousDragPos = Constants::NullDragPoint;
        m_dragOffsetX = 0;
        m_dragOffsetY = 0;
    }
}

bool isSelectedPixelClicked(QList<QPoint>& selectedPixels, QPoint& mouseLocation)
{
    for(QPoint p : selectedPixels)
    {
        if(p.x() == mouseLocation.x() && p.y() == mouseLocation.y())
        {
            return true;
        }
    }
    return false;
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bMouseDown)
    {
        m_canvasMutex.lock();

        QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(event->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);

        if(m_tool == TOOL_PAINT)
        {
            paintRect(m_canvasImage, mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor(), m_pParent->getBrushSize());
            update();
        }
        else if(m_tool == TOOL_ERASER)
        {
            paintRect(m_canvasImage, mouseLocation.x(), mouseLocation.y(), Qt::transparent, m_pParent->getBrushSize());
            update();
        }
        else if(m_tool == TOOL_SELECT)
        {
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
                m_panOffsetX += mouseLocation.x() - m_previousPanPos.x();
                m_panOffsetY += mouseLocation.y() - m_previousPanPos.y();

                m_previousPanPos = mouseLocation;

                update();                
            }
        }
        else if(m_tool == TOOL_DRAG)
        {
            //If starting dragging
            if(m_previousDragPos == Constants::NullDragPoint)
            {
                //check if mouse is over selection area
                if(isSelectedPixelClicked(m_selectedPixels, mouseLocation))
                {
                    if(m_clipboardImage == QImage())
                        m_clipboardImage = generateClipBoard(m_canvasImage, m_selectedPixels);

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
        else if(m_tool == TOOL_SHAPE)
        {
            const int xPos = m_drawShapeOrigin.x() < mouseLocation.x() ? m_drawShapeOrigin.x() : mouseLocation.x();
            const int yPos = m_drawShapeOrigin.y() < mouseLocation.y() ? m_drawShapeOrigin.y() : mouseLocation.y();

            int xLen = m_drawShapeOrigin.x() - mouseLocation.x();
            if (xLen < 0)
                xLen *= -1;

            int yLen = m_drawShapeOrigin.y() - mouseLocation.y();
            if (yLen < 0)
                yLen *= -1;

            const QRect rect = QRect(0,0,xLen,yLen);

            m_dragOffsetX = xPos;
            m_dragOffsetY = yPos;

            m_clipboardImage = QImage(QSize(xLen, yLen), QImage::Format_ARGB32);
            QPainter dragPainter(&m_clipboardImage);
            dragPainter.setCompositionMode (QPainter::CompositionMode_Clear);
            dragPainter.fillRect(m_clipboardImage.rect(), Qt::transparent);

            dragPainter.setRenderHint(QPainter::HighQualityAntialiasing, true);

            dragPainter.setCompositionMode (QPainter::CompositionMode_Source);

            if(m_pParent->getCurrentShape() == SHAPE_RECT)
            {
                if(m_pParent->getIsFillShape())
                {
                    dragPainter.fillRect(rect, m_pParent->getSelectedColor());
                }
                else
                {
                    QPen p;
                    p.setWidth(m_pParent->getBrushSize());
                    p.setColor(m_pParent->getSelectedColor());
                    dragPainter.setPen(p);
                    dragPainter.drawRect(rect);
                }

            }
            else if(m_pParent->getCurrentShape() == SHAPE_CIRCLE)
            {
                QPen p;
                if(m_pParent->getIsFillShape())
                {
                    dragPainter.setBrush(m_pParent->getSelectedColor());
                }
                else
                {
                    p.setWidth(m_pParent->getBrushSize());
                }
                p.setColor(m_pParent->getSelectedColor());
                dragPainter.setPen(p);
                dragPainter.drawEllipse(rect);
            }
            else if(m_pParent->getCurrentShape() == SHAPE_TRIANGLE)
            {
                QPainterPath path;
                const QPoint topMiddle = QPoint(rect.left() + rect.width()/2, rect.top());
                path.moveTo(topMiddle);
                path.lineTo(rect.bottomLeft());
                path.lineTo(rect.bottomRight());
                path.lineTo(topMiddle);

                if(m_pParent->getIsFillShape())
                {
                    dragPainter.fillPath(path, m_pParent->getSelectedColor());
                }
                else
                {
                    dragPainter.setPen(QPen(m_pParent->getSelectedColor(),m_pParent->getBrushSize()));
                    dragPainter.drawPath(path);
                }
            }

            update();

        }

        m_canvasMutex.unlock();
    }
}

void Canvas::updateCenter()
{
    m_center = QPoint(geometry().width() / 2, geometry().height() / 2);
}
