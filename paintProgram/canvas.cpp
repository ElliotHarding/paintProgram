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
const int SelectedPixelsOutlineFlashFrequency = 200;
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Canvas
///
Canvas::Canvas(MainWindow* parent, QImage image) :
    QTabWidget(),
    m_pParent(parent)
{
    CanvasLayer canvasLayer;
    canvasLayer.m_image = image;
    m_canvasLayers.push_back(canvasLayer);
    m_selectedLayer = 0;

    QList<CanvasLayerInfo> layers;
    layers.push_back(CanvasLayerInfo());//Todo - based of m_canvasLayers
    m_pParent->setLayers(layers);

    m_canvasWidth = image.width();
    m_canvasHeight = image.height();

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

void Canvas::onAddedToTab()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    updateCenter();

    const float x = float(geometry().width()) / float(m_canvasWidth);
    const float y = float(geometry().height()) / float(m_canvasHeight);
    m_zoomFactor = x < y ? x : y;
    if(m_zoomFactor < float(0.1))
        m_zoomFactor = 0.1;

    m_panOffsetX = m_center.x() - (m_canvasWidth / 2);
    m_panOffsetY = m_center.y() - (m_canvasHeight / 2);

    m_textDrawLocation = QPoint(m_canvasWidth / 2, m_canvasHeight / 2);

    recordImageHistory();
}

int Canvas::width()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasWidth;
}

int Canvas::height()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasHeight;
}

void Canvas::onUpdateText(QFont font)
{
    onWriteText("", font);
}

void Canvas::onWriteText(QString letter, QFont font)
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

        QImage textImage = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);
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

void Canvas::onLayerAdded()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);  

    CanvasLayer canvasLayer;
    canvasLayer.m_image = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);
    canvasLayer.m_image.fill(Qt::transparent);
    m_canvasLayers.push_back(canvasLayer);
}

void Canvas::onLayerDeleted(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers.removeAt(index);
}

void Canvas::onLayerEnabledChanged(const uint index, const bool enabled)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers[index].m_info.m_enabled = enabled; //Assumes there is a layer at index
}

void Canvas::onLayerTextChanged(const uint index, QString text)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers[index].m_info.m_name = text; //Assumes there is a layer at index
}

void Canvas::onSelectedLayerChanged(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_selectedLayer = index;
}

void Canvas::onUpdateSettings(int width, int height, QString name)
{
    m_canvasMutex.lock();

    m_canvasWidth = width;
    m_canvasHeight = height;

    for(CanvasLayer canvasLayer : m_canvasLayers)
    {
        //Create new image based on new settings
        QImage newImage = QImage(QSize(width, height), QImage::Format_ARGB32);

        //Fill new image as transparent
        newImage.fill(Qt::transparent);

        //Paint old image onto new image
        QPainter painter(&newImage);
        painter.setCompositionMode (QPainter::CompositionMode_Source);
        painter.drawImage(canvasLayer.m_image.rect(), canvasLayer.m_image);

        canvasLayer.m_image = newImage;
    }

    m_canvasBackgroundImage = genTransparentPixelsBackground(width, height);

    emit canvasSizeChange(width, height);

    //Clear selected pixels
    m_pSelectedPixels->clearAndResize(width, height);

    m_canvasMutex.unlock();

    if(m_savePath != "")
    {
        QFileInfo info(m_savePath);
        m_savePath = info.path() + "/" + name + "." + info.completeSuffix();
        qDebug() << m_savePath;
    }
}

void Canvas::onCurrentToolUpdated(const Tool t)
{
    bool doUpdate = false;

    if(m_tool == TOOL_TEXT && t != TOOL_TEXT)
        m_textToDraw = "";

    m_tool = t;

    if(m_tool != TOOL_DRAG)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        //Dump dragged contents onto m_canvasImage
        //If something actually dumps, record image history
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image); //Assumes there is a selectedLayer
        if(m_pClipboardPixels->dumpImage(painter))
        {
            recordImageHistory();
        }

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

void Canvas::onDeleteKeyPressed()
{
    if(m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        QMutexLocker canvasMutexLocker(&m_canvasMutex);

        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image); //Assumes there is a selected layer
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

void Canvas::onCopyKeysPressed()
{   
    m_canvasMutex.lock();
    Clipboard clipboard;
    clipboard.generateClipboard(m_canvasLayers[m_selectedLayer].m_image, m_pSelectedPixels); //Assumes there is a selected layer
    m_pParent->setCopyBuffer(clipboard);
    m_canvasMutex.unlock();

    update();
}

void Canvas::onCutKeysPressed()
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
        clipBoard.m_clipboardImage = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);

        QPainter dragPainter(&clipBoard.m_clipboardImage);
        dragPainter.setCompositionMode (QPainter::CompositionMode_Source);
        dragPainter.fillRect(clipBoard.m_clipboardImage.rect(), Qt::transparent);

        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image); //Assumes there is a selected layer
        painter.setCompositionMode (QPainter::CompositionMode_Clear);

        //Go through selected pixels cutting from canvas and copying to clipboard
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            dragPainter.fillRect(QRect(x, y, 1, 1), m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y)); //Assumes there is a selected layer
            painter.fillRect(QRect(x, y, 1, 1), Qt::transparent);
            clipBoard.m_pixels.push_back(QPoint(x,y));
        });
    }

    //Reset
    m_pClipboardPixels->reset();
    m_pSelectedPixels->clear();
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    recordImageHistory();

    canvasMutexLocker.unlock();

    m_pParent->setCopyBuffer(clipBoard);
    update();
}

void Canvas::onPasteKeysPressed()
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

void Canvas::onUndoPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(m_imageHistoryIndex > 0)
    {
        m_canvasLayers[m_selectedLayer].m_image = m_imageHistory[size_t(--m_imageHistoryIndex)]; //TODO ~ Make undo for all layers + this assumes there is a selected layer

        update();
    }
}

void Canvas::onRedoPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(m_imageHistoryIndex < int(m_imageHistory.size() - 1))
    {
        m_canvasLayers[m_selectedLayer].m_image = m_imageHistory[size_t(++m_imageHistoryIndex)]; //TODO ~ Make undo for all layers + this assumes there is a selected layer

        update();
    }
}

void operateOnCanvasPixels(QImage& canvas, std::function<void (int, int)> func)
{
    for(int x = 0; x < canvas.width(); x++)
    {
        for(int y = 0; y < canvas.height(); y++)
        {
            func(x, y);
        }
    }
}

QColor greyScaleColor(const QColor col)
{
    const int grey = (col.red() + col.green() + col.blue())/3;
    return QColor(grey, grey, grey, col.alpha());
}

void Canvas::onBlackAndWhite()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels, turning to white&black
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, greyScaleColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y))); //Assumes there is a selected layer
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, greyScaleColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y))); //Assumes there is a selected layer
        });
    }

    recordImageHistory();

    update();
}

QColor invertColor(const QColor col)
{
    return QColor(255 - col.red(), 255 - col.green(), 255 - col.blue(), col.alpha());
}

void Canvas::onInvert() // todo make option to invert alpha aswell
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, invertColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y))); //Assumes there is a selected layer
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, invertColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y)));
        });
    }

    recordImageHistory();

    update();
}

//Returns if neighbour pixel is not a similar color
bool compareNeighbour(QImage& image, const int x, const int y, const int neighbourX, const int neighbourY, int sensitivity)
{
    const QColor pixelColor = image.pixelColor(x,y);
    const QColor neighbourPixel = image.pixelColor(neighbourX,neighbourY);
    if ((pixelColor.red() >= neighbourPixel.red() + sensitivity) ||
        (pixelColor.red() <= neighbourPixel.red() - sensitivity) ||
        (pixelColor.green() <= neighbourPixel.green() - sensitivity) ||
        (pixelColor.green() >= neighbourPixel.green() + sensitivity) ||
        (pixelColor.blue() <= neighbourPixel.blue() - sensitivity) ||
        (pixelColor.blue() >= neighbourPixel.blue() + sensitivity) ||
        (pixelColor.alpha() <= neighbourPixel.alpha() - sensitivity) ||
        (pixelColor.alpha() >= neighbourPixel.alpha() + sensitivity)
        )
    {
        return true;
    }
    return false;
}

void Canvas::onSketchEffect(const int sensitivity)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects(); //Assumes there is a selected layer

    if(sensitivity == 0)
    {
        update();
        return;
    }

    QImage inkSketch = QImage(QSize(m_canvasLayers[m_selectedLayer].m_image.width(), m_canvasLayers[m_selectedLayer].m_image.height()), QImage::Format_ARGB32);
    const QColor sketchColor = m_pParent->getSelectedColor() != Qt::white ? m_pParent->getSelectedColor() : Qt::black;

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        inkSketch.fill(Qt::transparent);

        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x+1, y, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x-1, y, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y+1, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y-1, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else
            {
                inkSketch.setPixelColor(x, y, Qt::white);
            }
        });

        QPainter sketchPainter(&m_canvasLayers[m_selectedLayer].m_image);
        sketchPainter.drawImage(0,0,inkSketch);
    }
    else
    {
        inkSketch.fill(Qt::white);

        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x+1, y, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x-1, y, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y+1, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y-1, sensitivity))
            {
                inkSketch.setPixelColor(x, y, sketchColor);
            }
        });

        m_canvasLayers[m_selectedLayer].m_image = inkSketch;
    }

    update();
}

void Canvas::onOutlineEffect(const int sensitivity)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    if(sensitivity == 0)
    {
        update();
        return;
    }

    QImage outlineSketch = QImage(QSize(m_canvasLayers[m_selectedLayer].m_image.width(), m_canvasLayers[m_selectedLayer].m_image.height()), QImage::Format_ARGB32);
    //Laying this ontop of m_canvasImage so want most of it transparent
    outlineSketch.fill(Qt::transparent);

    const QColor sketchColor = m_pParent->getSelectedColor();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x+1, y, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x-1, y, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y+1, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y-1, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
        });
    }
    else
    {
        //Loop through pixels, if a border pixel set it to sketchColor
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x+1, y, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x-1, y, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y+1, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
            else if(compareNeighbour(m_canvasLayers[m_selectedLayer].m_image, x, y, x, y-1, sensitivity))
            {
                outlineSketch.setPixelColor(x, y, sketchColor);
            }
        });
    }

    //Dump outline sketch onto m_canvasImage
    QPainter sketchPainter(&m_canvasLayers[m_selectedLayer].m_image);
    sketchPainter.drawImage(0,0,outlineSketch);

    update();
}

int limitRange255(int num)
{
    if(num > 255)
        num = 255;
    if(num < 0)
        num = 0;
    return num;
}

QColor changeBrightness(QColor col, const int value)
{
    return QColor(limitRange255(col.red() + value), limitRange255(col.green() + value), limitRange255(col.blue() + value), col.alpha());
}

void Canvas::onBrightness(const int value)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeBrightness(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeBrightness(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }

    update();
}


int changeContrastRGOB(int rgob, const int value) // rgob --> stands for red, green or blue
{
    //127.5 is middle of 0 and 255, dulling contrast(<0) moves towards 127, high contrast(>0) moves away.

    if(rgob > 127)
    {
        if(value > 0)//Trying to move away from 127
        {
            rgob = limitRange255(rgob + value);
        }
        else if(value < 0)//Trying to get to 127
        {
            rgob += value; //wont go out of 0 255 range because rgob is bigger than 127 and subtracting something smaller or equal to 127
            if(rgob < 127)
                rgob = 127;
        }
    }
    else if(rgob < 127)//Less than 127
    {
        if(value > 0)//Trying to move away from 127
        {
            rgob = limitRange255(rgob - value);
        }
        else if(value < 0)//Trying to get to 127
        {
            rgob -= value;//wont go out of 0 255 range because rgob is smaller than 127 and adding something smaller or equal to 127 (adding because to negatives make positive)
            if(rgob > 127)//if pass 127 go back to 127
                rgob = 127;
        }
    }

    return rgob;
}

QColor changeContrast(QColor col, const int value)
{
    return QColor(changeContrastRGOB(col.red(), value), changeContrastRGOB(col.green(), value), changeContrastRGOB(col.blue(), value), col.alpha());
}

void Canvas::onContrast(const int value)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeContrast(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeContrast(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }

    update();
}

int limitInt(int value, const int limit)
{
    if(value > limit)
    {
        value = limit;
    }
    return value;
}

QColor limitRed(QColor col, const int limit)
{
    return QColor(limitInt(col.red(), limit), col.green(), col.blue(), col.alpha());
}

void Canvas::onRedLimit(const int value)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitRed(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitRed(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }

    update();
}

QColor limitBlue(QColor col, const int limit)
{
    return QColor(col.red(), col.green(), limitInt(col.blue(), limit), col.alpha());
}

void Canvas::onBlueLimit(const int value)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitBlue(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitBlue(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }

    update();
}

QColor limitGreen(QColor col, const int limit)
{
    return QColor(col.red(), limitInt(col.green(), limit), col.blue(), col.alpha());
}

void Canvas::onGreenLimit(const int value)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Get backup of canvas image before effects were applied (create backup if first effect)
    m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

    //check if were doing the whole image or just some selected pixels
    if(m_pSelectedPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitGreen(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitGreen(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }

    update();
}

void Canvas::onConfirmEffects()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_beforeEffectsImage = QImage();
    recordImageHistory();
}

void Canvas::onCancelEffects()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers[m_selectedLayer].m_image = m_beforeEffectsImage;
    m_beforeEffectsImage = QImage();
}

QImage Canvas::getImageCopy()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_canvasLayers[m_selectedLayer].m_image; //TODO - make for all layers?
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

    m_panOffsetX = m_center.x() - (m_canvasWidth / 2);
    m_panOffsetY = m_center.y() - (m_canvasHeight / 2);

    if(m_textDrawLocation.x() > (int)m_canvasWidth || m_textDrawLocation.y() > (int)m_canvasHeight)
        m_textDrawLocation = QPoint(m_canvasWidth / 2, m_canvasHeight / 2);

    update();
}

//Function called when m_canvasMutex is locked
void Canvas::recordImageHistory()
{
    m_imageHistory.push_back(m_canvasLayers[m_selectedLayer].m_image); //TODO - make for all layers - at the moment it assumes theres always a selected layer

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

    //Draw current layers
    for(CanvasLayer& canvasLayer : m_canvasLayers)
    {
        if(canvasLayer.m_info.m_enabled)
        {
            painter.drawImage(m_panOffsetX, m_panOffsetY, canvasLayer.m_image);
        }
    }

    //Draw selection tool
    if(m_tool == TOOL_SELECT)
    {
        //TODO ~ If highlight selection color and background color are the same we wont see highlighted area...
        painter.setPen(QPen(Constants::SelectionBorderColor, 1/m_zoomFactor));
        painter.drawRect(m_selectionTool->geometry().translated(m_panOffsetX, m_panOffsetY));
    }

    //Draw border
    painter.setPen(QPen(Constants::ImageBorderColor, 1/m_zoomFactor));
    painter.drawRect(QRect(0, 0, m_canvasWidth, m_canvasHeight).translated(m_panOffsetX, m_panOffsetY));

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
        if(m_zoomFactor < geometry().width())
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
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    QList<CanvasLayerInfo> layers;
    for(CanvasLayer& layer : m_canvasLayers)
    {
        layers.push_back(layer.m_info);
    }
    m_pParent->setLayers(layers);

    canvasMutexLocker.unlock();

    emit canvasSizeChange(m_canvasWidth, m_canvasHeight);
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

void paintBrush(QImage& canvas, const uint x, const uint y, const QColor col, const uint widthHeight, const BrushShape brushShape)
{
    if(x <= (uint)canvas.width() && y <= (uint)canvas.height())
    {
        QPainter painter(&canvas);
        painter.setCompositionMode (QPainter::CompositionMode_Source);

        QRect rect;
        if(widthHeight > 1)
        {
            rect = QRect(x - widthHeight/2, y - widthHeight/2, widthHeight, widthHeight);
        }
        else
        {
            rect = QRect(x, y, widthHeight, widthHeight);
        }

        if(brushShape == BrushShape::BRUSHSHAPE_CIRCLE && widthHeight > 1)
        {
            QPen p;
            p.setColor(col);
            painter.setPen(p);
            painter.setBrush(col);
            painter.drawEllipse(rect);
        }
        else if(brushShape == BrushShape::BRUSHSHAPE_RECT || (widthHeight <= 1 && brushShape == BrushShape::BRUSHSHAPE_CIRCLE))
        {
            painter.fillRect(rect, col);
        }
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
        //Dump clipboard, if something actually dumped record image history
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);
        if(m_pClipboardPixels->dumpImage(painter))
        {
            recordImageHistory();
        }
        update();
    }

    if(m_tool == TOOL_PAINT)
    {
        paintBrush(m_canvasLayers[m_selectedLayer].m_image, mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor(), m_pParent->getBrushSize(), m_pParent->getCurrentBrushShape());
        update();
    }
    else if(m_tool == TOOL_ERASER)
    {
        paintBrush(m_canvasLayers[m_selectedLayer].m_image, mouseLocation.x(), mouseLocation.y(), Qt::transparent, m_pParent->getBrushSize(), m_pParent->getCurrentBrushShape());
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

        std::vector<std::vector<bool>> newSelectedPixels = std::vector<std::vector<bool>>(m_canvasWidth, std::vector<bool>(m_canvasHeight, false));
        spreadSelectSimilarColor(m_canvasLayers[m_selectedLayer].m_image, newSelectedPixels, mouseLocation, m_pParent->getSpreadSensitivity());
        m_pSelectedPixels->addPixels(newSelectedPixels);

        update();
    }
    else if(m_tool == TOOL_BUCKET)
    {
        floodFillOnSimilar(m_canvasLayers[m_selectedLayer].m_image, m_pParent->getSelectedColor(), mouseLocation.x(), mouseLocation.y(), m_pParent->getSpreadSensitivity());

        update();
    }
    else if(m_tool == TOOL_COLOR_PICKER)
    {
        m_pParent->setSelectedColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(mouseLocation.x(), mouseLocation.y()));
    }
    else if(m_tool == TOOL_TEXT)
    {
        m_textDrawLocation = mouseLocation;

        canvasMutexLocker.unlock();

        onUpdateText(m_pParent->getTextFont());

        canvasMutexLocker.relock();
    }
    else if(m_tool == TOOL_SHAPE)
    {
        //Dump dragged contents onto m_canvasImage
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);
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

void Canvas::onParentMouseMove(QMouseEvent *event)
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
            paintBrush(m_canvasLayers[m_selectedLayer].m_image, mouseLocation.x(), mouseLocation.y(), m_pParent->getSelectedColor(), m_pParent->getBrushSize(), m_pParent->getCurrentBrushShape());
            update();
        }
        else if(m_tool == TOOL_ERASER)
        {
            paintBrush(m_canvasLayers[m_selectedLayer].m_image, mouseLocation.x(), mouseLocation.y(), Qt::transparent, m_pParent->getBrushSize(), m_pParent->getCurrentBrushShape());
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
                        m_pClipboardPixels->generateClipboard(m_canvasLayers[m_selectedLayer].m_image, m_pSelectedPixels);
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

            QImage newShapeImage = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);

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

//Requires m_canvasMutex to be locked!
QImage Canvas::getCanvasImageBeforeEffects()
{
    if(m_beforeEffectsImage == QImage())
    {
        m_beforeEffectsImage = m_canvasLayers[m_selectedLayer].m_image; //Assumes there is a selected layer
    }
    return m_beforeEffectsImage;
}

void Canvas::updateCenter()
{
    m_center = QPoint(geometry().width() / 2, geometry().height() / 2);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SelectedPixels
///
SelectedPixels::SelectedPixels(Canvas* parent, const uint width, const uint height) : QWidget(parent),
    m_pParentCanvas(parent)
{
    m_selectedPixels = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));

    setGeometry(0, 0, parent->width(), parent->height());

    m_pOutlineDrawTimer = new QTimer(this);
    connect(m_pOutlineDrawTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_pOutlineDrawTimer->start(Constants::SelectedPixelsOutlineFlashFrequency);
}

SelectedPixels::~SelectedPixels()
{
    if(m_pOutlineDrawTimer)
        delete m_pOutlineDrawTimer;
}

void SelectedPixels::clearAndResize(const uint width, const uint height)
{
    m_selectedPixels = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));
    m_selectedPixelsList.clear();
}

void SelectedPixels::clear()
{
    m_selectedPixelsList.clear();
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
    for(QPoint p : m_selectedPixelsList)
    {
        func(p.x(), p.y());
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
                if(!m_selectedPixels[x][y])
                {
                    m_selectedPixelsList.push_back(QPoint(x,y));
                    m_selectedPixels[x][y] = true;
                }
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
    //Check m_selectedPixels is the same dimensions as selectedPixels
    //Could implement a method to add sections of pixels to m_selectedPixels, not needed right now
    if(!(m_selectedPixels.size() > 0 && selectedPixels.size() > 0 &&
       m_selectedPixels.size() == selectedPixels.size() &&
       m_selectedPixels[0].size() == selectedPixels[0].size()))
    {
        qDebug() << "SelectedPixels::addPixels - 2D vector added is not the same as m_selectedPixels vector";
        return;
    }

    for(uint x = 0; x < m_selectedPixels.size(); x++)
    {
        for(uint y = 0; y < m_selectedPixels[x].size(); y++)
        {
            if(selectedPixels[x][y])
            {
                if(!m_selectedPixels[x][y])
                {
                    m_selectedPixelsList.push_back(QPoint(x,y));
                    m_selectedPixels[x][y] = true;
                }

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
            if(!m_selectedPixels[p.x()][p.y()])
            {
                m_selectedPixelsList.push_back(QPoint(p.x(), p.y()));
                m_selectedPixels[p.x()][p.y()] = true;
            }
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

bool SelectedPixels::containsPixels()
{
    return m_selectedPixelsList.size() > 0;
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
    for(QPoint p : m_selectedPixelsList)
    {
        const uint x = p.x();
        const uint y = p.y();

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Clipboard
///
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PaintableClipboard
///
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

//Returns false if no image dumped
bool PaintableClipboard::dumpImage(QPainter &painter)
{
    if(m_clipboardImage == QImage() && m_pixels.size() == 0)
    {
        //Just in case
        update();
        return false;
    }

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
    return true;
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

    //Draw transparent selected pixels ~ So inneficient! look for something else
    for(QPoint p : m_pixels)
    {
        if(m_clipboardImage.pixelColor(p.x(), p.y()).alpha() == 0)
        {
            const QColor col = (p.x() % 2 == 0) ?
                        (p.y() % 2 == 0) ? Constants::TransparentWhite : Constants::TransparentGrey
                                     :
                        (p.y() % 2 == 0) ? Constants::TransparentGrey : Constants::TransparentWhite;

            painter.fillRect(QRect(p.x() + m_dragX + offsetX, p.y() + m_dragY + offsetY, 1, 1), col);
        }
    }
}
