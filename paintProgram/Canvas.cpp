#include "canvas.h"
#include <QWheelEvent>
#include <QDebug>
#include <QSet>
#include <QFileInfo>
#include <stack>
#include <QPainterPath>

#include "mainwindow.h"

namespace Constants
{
const QPoint NullDragPoint = QPoint(0,0);
const QColor ImageBorderColor = QColor(200,200,200,255);
const QColor TransparentGrey = QColor(190,190,190,255);
const QColor TransparentWhite = QColor(255,255,255,255);
const QColor SelectionBorderColor = Qt::blue;

const QColor SelectionAreaColorA = QColor(0,40,100,50);
}

QImage genTransparentPixelsBackground(const int width, const int height)
{
    //TODO ~ Test if quicker to paint all white, then fill grey squares after
    QImage transparentBackground = QImage(QSize(width, height), QImage::Format_ARGB32);
    QPainter painter(&transparentBackground);
    for(int x = 0; x < width; x++)
    {
        for(int y = 0; y < height; y++)
        {
            const QColor col = (x % 2 == 0) ?
                        (y % 2 == 0) ? Constants::TransparentWhite : Constants::TransparentGrey
                                     :
                        (y % 2 == 0) ? Constants::TransparentGrey : Constants::TransparentWhite;

            painter.fillRect(QRect(x, y, 1, 1), col);
        }
    }

    return transparentBackground;
}

Canvas::Canvas(MainWindow* parent, QImage image) :
    QTabWidget(),
    m_pParent(parent)
{
    m_canvasImage = image;    
    m_canvasBackgroundImage = genTransparentPixelsBackground(image.width(), image.height());

    m_selectionTool = new QRubberBand(QRubberBand::Rectangle, this);
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    m_pSelectedPixels = new SelectedPixels(this, image.width(), image.height());
    m_pSelectedPixels->raise();

    m_pClipboardPixels = new PaintableClipboard(this);
    m_pClipboardPixels->raise();

    setMouseTracking(true);
}

Canvas::~Canvas()
{
    if(m_selectionTool)
        delete m_selectionTool;

    if(m_pSelectedPixels)
        delete m_pSelectedPixels;
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
        if(letter == "\010")//backspace
        {
            m_textToDraw.chop(1);
        }
        else
        {
            m_textToDraw += letter;
        }

        m_canvasMutex.lock();

        QImage textImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);
        QPainter textPainter(&textImage);
        textPainter.setCompositionMode (QPainter::CompositionMode_Clear);
        textPainter.fillRect(textImage.rect(), Qt::transparent);
        textPainter.setCompositionMode (QPainter::CompositionMode_Source);
        textPainter.setPen(m_pParent->getSelectedColor());
        textPainter.setFont(font);
        textPainter.drawText(m_textDrawLocation, m_textToDraw);

        m_pClipboardPixels->setImage(textImage);
        m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixels());

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
    m_canvasBackgroundImage = genTransparentPixelsBackground(width, height);

    emit canvasSizeChange(m_canvasImage.width(), m_canvasImage.height());

    //Clear selected pixels
    m_pSelectedPixels->clearAndResize(m_canvasImage.width(), m_canvasImage.height());

    m_canvasMutex.unlock();

    if(m_savePath != "")
    {
        QFileInfo info(m_savePath);
        m_savePath = info.path() + "/" + name + "." + info.completeSuffix();
        qDebug() << m_savePath;
    }
}

void Canvas::updateCurrentTool(const Tool t)
{
    bool doUpdate = false;

    if(m_tool == TOOL_TEXT && t != TOOL_TEXT)
        m_textToDraw = "";

    m_tool = t;

    if(m_tool != TOOL_DRAG)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        //Dump dragged contents onto m_canvasImage
        QPainter painter(&m_canvasImage);
        m_pClipboardPixels->dumpImage(painter);

        recordImageHistory();

        canvasMutexLocker.unlock();

        doUpdate = true;
    }

    if(m_tool != TOOL_SELECT)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        //Reset selection rectangle tool        
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

        emit selectionAreaResize(0,0);

        if(m_tool != TOOL_SPREAD_ON_SIMILAR && m_tool != TOOL_DRAG)
        {
            //Clear selected pixels
            m_pSelectedPixels->clear();
        }

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

        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            painter.fillRect(QRect(x, y, 1, 1), Qt::transparent);
        });

        m_pSelectedPixels->clear();

        recordImageHistory();

        canvasMutexLocker.unlock();

        update();
    }
}

void Canvas::copyKeysPressed()
{   
    m_canvasMutex.lock();
    Clipboard clipboard;
    clipboard.generateClipboard(m_canvasImage, m_pSelectedPixels);
    m_pParent->setCopyBuffer(clipboard);
    m_canvasMutex.unlock();

    update();
}

void Canvas::cutKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    Clipboard clipBoard;

    //What if already dragging something around?
    if(m_pClipboardPixels->isDragging())
    {
        clipBoard = m_pClipboardPixels->getClipboard();
    }
    else
    {
        //Copy cut pixels to clipboard
        clipBoard.m_clipboardImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);

        QPainter dragPainter(&clipBoard.m_clipboardImage);
        dragPainter.setCompositionMode (QPainter::CompositionMode_Source);
        dragPainter.fillRect(clipBoard.m_clipboardImage.rect(), Qt::transparent);

        QPainter painter(&m_canvasImage);
        painter.setCompositionMode (QPainter::CompositionMode_Clear);

        //Go through selected pixels cutting from canvas and copying to clipboard
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            dragPainter.fillRect(QRect(x, y, 1, 1), m_canvasImage.pixelColor(x, y));
            painter.fillRect(QRect(x, y, 1, 1), Qt::transparent);
            clipBoard.m_pixels.push_back(QPoint(x,y));
        });
    }

    //Reset
    m_pClipboardPixels->reset();
    m_pSelectedPixels->clear();

    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    canvasMutexLocker.unlock();

    recordImageHistory();

    m_pParent->setCopyBuffer(clipBoard);
    update();
}

void Canvas::pasteKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    m_pClipboardPixels->reset();

    m_pClipboardPixels->setClipboard(m_pParent->getCopyBuffer());

    m_pSelectedPixels->clear();
    m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixels());

    m_pSelectedPixels->raise();

    canvasMutexLocker.unlock();
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

float Canvas::getZoom()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_zoomFactor;
}

void Canvas::getPanOffset(float& offsetX, float& offsetY)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    offsetX = m_panOffsetX;
    offsetY = m_panOffsetY;
}

void Canvas::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);

    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    updateCenter();

    m_pSelectedPixels->setGeometry(geometry());
    m_pClipboardPixels->setGeometry(geometry());

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
    painter.drawImage(m_panOffsetX, m_panOffsetY, m_canvasBackgroundImage);

    //Draw current image
    painter.drawImage(m_panOffsetX, m_panOffsetY, m_canvasImage);

    //Draw selection tool
    if(m_tool == TOOL_SELECT)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(Constants::SelectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry().translated(m_panOffsetX, m_panOffsetY));
    }

    //Draw border
    painter.setPen(QPen(Constants::ImageBorderColor, 1/m_zoomFactor));
    painter.drawRect(m_canvasImage.rect().translated(m_panOffsetX, m_panOffsetY));

    m_canvasMutex.unlock();
}

void Canvas::wheelEvent(QWheelEvent* event)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    const int direction = event->angleDelta().y() > 0 ? 1 : -1;

    const int xFromCenter = event->position().x() - m_center.x();
    m_panOffsetX -= xFromCenter * 0.05 * direction / m_zoomFactor;

    const int yFromCenter = event->position().y() - m_center.y();
    m_panOffsetY -= yFromCenter * 0.05 * direction / m_zoomFactor;

    if(event->angleDelta().y() > 0)
    {
        if(m_zoomFactor < m_canvasImage.width())
            m_zoomFactor *= (m_cZoomIncrement);
    }
    else if(event->angleDelta().y() < 0)
    {
        if(m_zoomFactor > 0.1)
            m_zoomFactor /= (m_cZoomIncrement);
    }

    //Call to redraw
    update();
}

void Canvas::showEvent(QShowEvent *)
{
    emit canvasSizeChange(m_canvasImage.width(), m_canvasImage.height());
    emit selectionAreaResize(0,0);
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
    if(x <= (uint)canvas.width() && y <= (uint)canvas.height())
    {
        QPainter painter(&canvas);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        QRect rect = QRect(x - widthHeight/2, y - widthHeight/2, widthHeight, widthHeight);
        painter.fillRect(rect, col);
    }
}

void spreadSelectSimilarColor(QImage& image, std::vector<std::vector<bool>>& selectedPixels, QPoint startPixel, int sensitivty)
{
    if(startPixel.x() > image.width() || startPixel.x() < 0 || startPixel.y() > image.height() || startPixel.y() < 0)
        return;

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
        if (selectedPixels[x][y] == false &&
            pixelColor.red() <= colorToSpreadOver.red() + sensitivty && pixelColor.red() >= colorToSpreadOver.red() - sensitivty &&
            pixelColor.green() <= colorToSpreadOver.green() + sensitivty && pixelColor.green() >= colorToSpreadOver.green() - sensitivty &&
            pixelColor.blue() <= colorToSpreadOver.blue() + sensitivty && pixelColor.blue() >= colorToSpreadOver.blue() - sensitivty &&
            pixelColor.alpha() <= colorToSpreadOver.alpha() + sensitivty && pixelColor.alpha() >= colorToSpreadOver.alpha() - sensitivty
            )
        {
            selectedPixels[x][y] = true;
            stack.push(QPoint(x + 1, y));
            stack.push(QPoint(x - 1, y));
            stack.push(QPoint(x, y + 1));
            stack.push(QPoint(x, y - 1));
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

    //If were not dragging, and the clipboard shows something. Dump it
    if(m_tool != TOOL_DRAG && !m_pClipboardPixels->isImageDefault())
    {
        QPainter painter(&m_canvasImage);
        m_pClipboardPixels->dumpImage(painter);
        update();
    }

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
            m_pSelectedPixels->clear();
        }

        m_selectionToolOrigin = mouseLocation;
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }
    else if(m_tool == TOOL_SPREAD_ON_SIMILAR)
    {        
        if(!m_pParent->isCtrlPressed())
        {
            m_pSelectedPixels->clear();
        }

        std::vector<std::vector<bool>> newSelectedPixels = std::vector<std::vector<bool>>(m_canvasImage.width(), std::vector<bool>(m_canvasImage.height(), false));
        spreadSelectSimilarColor(m_canvasImage, newSelectedPixels, mouseLocation, m_pParent->getSpreadSensitivity());
        m_pSelectedPixels->addPixels(newSelectedPixels);

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
        //Dump dragged contents onto m_canvasImage
        QPainter painter(&m_canvasImage);
        m_pClipboardPixels->dumpImage(painter);
        painter.end();

        update();

        recordImageHistory();

        m_pSelectedPixels->clear();

        m_drawShapeOrigin = mouseLocation;
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_bMouseDown = false;

    if(m_tool == TOOL_SELECT)
    {
        m_pSelectedPixels->addPixels(m_selectionTool);
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
        m_pSelectedPixels->clear();
        m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixels());

        update();
    }
    else if(m_tool == TOOL_DRAG)
    {
        m_pClipboardPixels->raise();
    }
}

void Canvas::mouseMouseOnParentEvent(QMouseEvent *event)
{
    m_canvasMutex.lock();

    QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(event->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);
    emit mousePositionChange(mouseLocation.x(), mouseLocation.y());

    m_canvasMutex.unlock();
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    m_canvasMutex.lock();

    QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(event->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);
    emit mousePositionChange(mouseLocation.x(), mouseLocation.y());

    if(m_bMouseDown)
    {
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
            emit selectionAreaResize(m_selectionTool->geometry().width(), m_selectionTool->geometry().height());
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
            if(!m_pClipboardPixels->isDragging())
            {
                //check if mouse is over selection area
                if(m_pSelectedPixels->isHighlighted(mouseLocation.x(), mouseLocation.y()))
                {
                    if(m_pClipboardPixels->isImageDefault())
                    {
                        m_pClipboardPixels->generateClipboard(m_canvasImage, m_pSelectedPixels);
                    }

                    m_pClipboardPixels->startDragging(mouseLocation);
                }
            }
            else //If currently dragging
            {
                m_pClipboardPixels->doDragging(mouseLocation);

                //Clear selected pixels and set to clipboard pixels
                m_pSelectedPixels->clear();
                m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixelsOffset());
            }
        }
        else if(m_tool == TOOL_SHAPE)
        {           
            m_pClipboardPixels->reset();

            QImage newShapeImage = QImage(QSize(m_canvasImage.width(), m_canvasImage.height()), QImage::Format_ARGB32);

            QPainter shapePainter(&newShapeImage);
            shapePainter.setCompositionMode (QPainter::CompositionMode_Clear);
            shapePainter.fillRect(newShapeImage.rect(), Qt::transparent);
            shapePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
            shapePainter.setCompositionMode (QPainter::CompositionMode_Source);

            if(m_pParent->getCurrentShape() == SHAPE_LINE)
            {
                //Draw line
                shapePainter.setPen(QPen(m_pParent->getSelectedColor(), m_pParent->getBrushSize()));
                shapePainter.drawLine(m_drawShapeOrigin, mouseLocation);
            }
            else
            {
                //Prep shape dimensions in form of rectangle
                const int xPos = m_drawShapeOrigin.x() < mouseLocation.x() ? m_drawShapeOrigin.x() : mouseLocation.x();
                const int yPos = m_drawShapeOrigin.y() < mouseLocation.y() ? m_drawShapeOrigin.y() : mouseLocation.y();
                int xLen = m_drawShapeOrigin.x() - mouseLocation.x();
                if (xLen < 0)
                    xLen *= -1;
                int yLen = m_drawShapeOrigin.y() - mouseLocation.y();
                if (yLen < 0)
                    yLen *= -1;
                const QRect rect = QRect(xPos,yPos,xLen,yLen);

                //Draw selected shape
                if(m_pParent->getCurrentShape() == SHAPE_RECT)
                {
                    if(m_pParent->getIsFillShape())
                    {
                        shapePainter.fillRect(rect, m_pParent->getSelectedColor());
                    }
                    else
                    {
                        QPen p;
                        p.setWidth(m_pParent->getBrushSize());
                        p.setColor(m_pParent->getSelectedColor());
                        p.setJoinStyle(Qt::MiterJoin);
                        shapePainter.setPen(p);
                        shapePainter.drawRect(rect);
                    }

                }
                else if(m_pParent->getCurrentShape() == SHAPE_CIRCLE)
                {
                    QPen p;
                    if(m_pParent->getIsFillShape())
                    {
                        shapePainter.setBrush(m_pParent->getSelectedColor());
                    }
                    else
                    {
                        p.setWidth(m_pParent->getBrushSize());
                    }
                    p.setColor(m_pParent->getSelectedColor());
                    shapePainter.setPen(p);
                    shapePainter.drawEllipse(rect);
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
                        shapePainter.fillPath(path, m_pParent->getSelectedColor());
                    }
                    else
                    {
                        QPen p;
                        p.setWidth(m_pParent->getBrushSize());
                        p.setColor(m_pParent->getSelectedColor());
                        p.setJoinStyle(Qt::MiterJoin);
                        shapePainter.setPen(p);
                        shapePainter.drawPath(path);
                    }
                }
            }

            shapePainter.end();
            m_pClipboardPixels->setImage(newShapeImage);
        }        
    }
    m_canvasMutex.unlock();
}

void Canvas::updateCenter()
{
    m_center = QPoint(geometry().width() / 2, geometry().height() / 2);
}




SelectedPixels::SelectedPixels(Canvas* parent, const uint width, const uint height) : QWidget(parent),
    m_pParentCanvas(parent)
{
    m_selectedPixels = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));

    setGeometry(0, 0, parent->width(), parent->height());

    m_pOutlineDrawTimer = new QTimer(this);
    connect(m_pOutlineDrawTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_pOutlineDrawTimer->start(200);
}

SelectedPixels::~SelectedPixels()
{
    if(m_pOutlineDrawTimer)
        delete m_pOutlineDrawTimer;
}

void SelectedPixels::clearAndResize(const uint width, const uint height)
{
    m_selectedPixels = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));
}

void SelectedPixels::clear()
{
    if(m_selectedPixels.size() > 0)
    {
        m_selectedPixels = std::vector<std::vector<bool>>(m_selectedPixels.size(), std::vector<bool>(m_selectedPixels[0].size(), false));
        update();
    }
    else
    {
        qDebug() << "SelectedPixels::clear - No pixels to clear";
    }

}

void SelectedPixels::operateOnSelectedPixels(std::function<void (int, int)> func)
{
    for(uint x = 0; x < m_selectedPixels.size(); x++)
    {
        for(uint y = 0; y < m_selectedPixels[x].size(); y++)
        {
            if(m_selectedPixels[x][y])
            {
                func(x,y);
            }
        }
    }
}

void SelectedPixels::addPixels(QRubberBand *newSelectionArea)
{
    if(newSelectionArea == nullptr)
        return;

    const QRect geometry = newSelectionArea->geometry();
    for (int x = geometry.x(); x < geometry.x() + geometry.width(); x++)
    {
        for (int y = geometry.y(); y < geometry.y() + geometry.height(); y++)
        {
            if(x > -1 && x < (int)m_selectedPixels.size() && y > -1 && y < (int)m_selectedPixels[0].size())
            {
                m_selectedPixels[x][y] = true;
            }
            else
            {
                qDebug() << "SelectedPixels::addPixels(QRubberBand *newSelectionArea) - out of range - " << x << ":" << y;
            }
        }
    }

    update();
}

void SelectedPixels::addPixels(std::vector<std::vector<bool>>& selectedPixels)
{
    for(uint x = 0; x < m_selectedPixels.size(); x++)
    {
        for(uint y = 0; y < m_selectedPixels[x].size(); y++)
        {
            if(!m_selectedPixels[x][y])
            {
                m_selectedPixels[x][y] = selectedPixels[x][y];
            }
        }
    }

    update();
}

void SelectedPixels::addPixels(QList<QPoint> pixels)
{
    for(QPoint p : pixels)
    {
        if(p.x() < (int)m_selectedPixels.size() && p.x() > -1 &&
           p.y() < (int)m_selectedPixels[0].size() && p.y() > -1)
        {
            m_selectedPixels[p.x()][p.y()] = true;
        }
    }

    update();
}

bool SelectedPixels::isHighlighted(const uint x, const uint y)
{
    if((int)x > -1 && x < m_selectedPixels.size() && (int)y > -1 && y < m_selectedPixels[0].size())
    {
        return m_selectedPixels[x][y];
    }
    else
    {
        qDebug() << "SelectedPixels::isHighlighted - Out of range -" << x << ":" << y;
    }
    return false;
}

void SelectedPixels::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(this);

    const QPoint center = QPoint(geometry().width() / 2, geometry().height() / 2);
    painter.translate(center);
    painter.scale(m_pParentCanvas->getZoom(), m_pParentCanvas->getZoom());
    painter.translate(-center);

    //Offsets
    float offsetX = 0;
    float offsetY = 0;
    m_pParentCanvas->getPanOffset(offsetX, offsetY);

    //Outline
    m_bOutlineColorToggle = !m_bOutlineColorToggle;
    QPen selectionOutlinePen = QPen(m_bOutlineColorToggle ? Qt::black : Qt::white, 1/m_pParentCanvas->getZoom());
    painter.setPen(selectionOutlinePen);

    //Paint pixels and outline
    for(uint x = 0; x < m_selectedPixels.size(); x++)
    {
        for(uint y = 0; y < m_selectedPixels[x].size(); y++)
        {
            if(m_selectedPixels[x][y])
            {
                painter.fillRect(QRect(x + offsetX, y + offsetY, 1, 1), Constants::SelectionAreaColorA);

                //border right
                if(x == m_selectedPixels.size()-1 || (x + 1 < m_selectedPixels.size() && !m_selectedPixels[x+1][y]))
                {
                    painter.drawLine(QPoint(x + offsetX + 1, y + offsetY), QPoint(x + offsetX + 1, y + offsetY + 1));
                }

                //border left
                if(x == 0 || (x > 0 && !m_selectedPixels[x-1][y]))
                {
                    painter.drawLine(QPoint(x + offsetX, y + offsetY), QPoint(x + offsetX, y + offsetY + 1));
                }

                //border bottom
                if(y == m_selectedPixels[x].size()-1 || (y + 1 < m_selectedPixels.size() && !m_selectedPixels[x][y+1]))
                {
                    painter.drawLine(QPoint(x + offsetX, y + offsetY + 1.0), QPoint(x + offsetX + 1.0, y + offsetY + 1.0));
                }

                //border top
                if(y == 0 || (y > 0 && !m_selectedPixels[x][y-1]))
                {
                    painter.drawLine(QPoint(x + offsetX, y + offsetY), QPoint(x + offsetX + 1.0, y + offsetY));
                }
            }
        }
    }
}



void Clipboard::generateClipboard(QImage &canvas, SelectedPixels* pSelectedPixels)
{
    //Prep selected pixels for dragging
    m_clipboardImage = QImage(QSize(canvas.width(), canvas.height()), QImage::Format_ARGB32);
    QPainter dragPainter(&m_clipboardImage);
    dragPainter.setCompositionMode (QPainter::CompositionMode_Clear);
    dragPainter.fillRect(m_clipboardImage.rect(), Qt::transparent);

    dragPainter.setCompositionMode (QPainter::CompositionMode_Source);

    m_pixels.clear();

    pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
    {
        dragPainter.fillRect(QRect(x, y, 1, 1), canvas.pixelColor(x,y));
        m_pixels.push_back(QPoint(x,y));
    });
}


PaintableClipboard::PaintableClipboard(Canvas* parent) : QWidget(parent),
    m_previousDragPos(Constants::NullDragPoint),
    m_pParentCanvas(parent)
{
    setGeometry(0, 0, parent->width(), parent->height());
}

void PaintableClipboard::generateClipboard(QImage &canvas, SelectedPixels *pSelectedPixels)
{
    Clipboard::generateClipboard(canvas, pSelectedPixels);
    update();
}

void PaintableClipboard::setClipboard(Clipboard clipboard)
{
    m_clipboardImage = clipboard.m_clipboardImage;
    m_pixels = clipboard.m_pixels;
    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = 0;
    m_dragY = 0;
    update();
}

Clipboard PaintableClipboard::getClipboard()
{
    Clipboard clipboard;
    clipboard.m_clipboardImage = m_clipboardImage;
    clipboard.m_pixels = m_pixels;
    return clipboard;
}

void PaintableClipboard::setImage(QImage image)
{
    m_clipboardImage = image;

    m_pixels.clear();
    for(int x = 0; x < image.width(); x++)
    {
        for(int y = 0; y < image.height(); y++)
        {
            if(image.pixelColor(x,y).alpha() > 0)
            {
                m_pixels.push_back(QPoint(x,y));
            }
        }
    }

    update();
}

void PaintableClipboard::dumpImage(QPainter &painter)
{
    //Draw image part of clipboard
    painter.drawImage(QRect(m_dragX, m_dragY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);

    //Draw transparent part of clipboard
    painter.setCompositionMode (QPainter::CompositionMode_Clear);
    for(QPoint p : m_pixels)
    {
        if(p.x() > 0 && p.x() < (int)m_clipboardImage.width() &&
           p.y()> 0 && p.y() < (int)m_clipboardImage.height() &&
           m_clipboardImage.pixelColor(p.x(), p.y()).alpha() == 0)
        {
            painter.fillRect(QRect(p.x() + m_dragX, p.y() + m_dragY, 1, 1), Qt::transparent);
        }
    }

    reset();
    update();
}

bool PaintableClipboard::isImageDefault()
{
    return m_clipboardImage == QImage();
}

QList<QPoint> PaintableClipboard::getPixels()
{
    return m_pixels;
}

QList<QPoint> PaintableClipboard::getPixelsOffset()
{
    QList<QPoint> pixels = m_pixels;
    for(QPoint& p : pixels)
    {
        p.setX(p.x() + m_dragX);
        p.setY(p.y() + m_dragY);
    }
    return pixels;
}

bool PaintableClipboard::isDragging()
{
    return (m_previousDragPos != Constants::NullDragPoint);
}

void PaintableClipboard::startDragging(QPoint mouseLocation)
{
    m_previousDragPos = mouseLocation;
    m_dragX = 0;
    m_dragY = 0;
}

void PaintableClipboard::doDragging(QPoint mouseLocation)
{
    m_dragX += (mouseLocation.x() - m_previousDragPos.x());
    m_dragY += (mouseLocation.y() - m_previousDragPos.y());

    m_previousDragPos = mouseLocation;

    update();
}

void PaintableClipboard::reset()
{
    m_clipboardImage = QImage();
    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = 0;
    m_dragY = 0;
    m_pixels.clear();
}

void PaintableClipboard::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(this);

    const QPoint center = QPoint(geometry().width() / 2, geometry().height() / 2);
    painter.translate(center);
    painter.scale(m_pParentCanvas->getZoom(), m_pParentCanvas->getZoom());
    painter.translate(-center);

    float offsetX = 0;
    float offsetY = 0;
    m_pParentCanvas->getPanOffset(offsetX, offsetY);

    painter.drawImage(QRect(m_dragX + offsetX, m_dragY + offsetY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);
}
