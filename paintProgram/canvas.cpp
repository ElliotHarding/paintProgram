#include "canvas.h"
#include <QWheelEvent>
#include <QDebug>
#include <QSet>
#include <QFileInfo>
#include <stack>
#include <QPainterPath>
#include <QBuffer>
#include <QFileInfo>
#include <cmath>
#include <math.h>

#include "mainwindow.h"

namespace Constants
{

//Colors
const QColor ImageBorderColor = QColor(200,200,200,255);
const QColor TransparentGrey = QColor(190,190,190,255);
const QColor TransparentWhite = QColor(255,255,255,255);
const QColor SelectionBorderColor = Qt::blue;
const QColor SelectionAreaColor = QColor(0,40,100,50);

//Drawing
const int SelectedPixelsOutlineFlashFrequency = 200;

//History-undo-redo
const uint MaxCanvasHistory = 20;

//Saving/loading
const QString CanvasSaveFileType = "paintProgram";
const QString CanvasSaveLayerBegin = "BEGIN_LAYER";
const QString CanvasSaveLayerEnd = "END_LAYER";

//Zooming
const float ZoomIncrement = 1.1;

//Dragging
const int DragNubbleSize = 8;
const QPoint NullDragPoint = QPoint(0,0);
}

QImage genTransparentPixelsBackground(const int width, const int height)
{
    //TODO ~ Test if quicker to paint all white, then fill grey squares after
    QImage transparentBackground = QImage(QSize(width, height), QImage::Format_ARGB32);
    for(int x = 0; x < width; x++)
    {
        for(int y = 0; y < height; y++)
        {
            const QColor col = (x % 2 == 0) ?
                        (y % 2 == 0) ? Constants::TransparentWhite : Constants::TransparentGrey
                                     :
                        (y % 2 == 0) ? Constants::TransparentGrey : Constants::TransparentWhite;
            transparentBackground.setPixelColor(x, y, col);
        }
    }
    return transparentBackground;
}

QList<CanvasLayerInfo> getLayerInfoList(QList<CanvasLayer>& canvasLayers)
{
    QList<CanvasLayerInfo> layers;
    for(CanvasLayer& layer : canvasLayers)
    {
        layers.push_back(layer.m_info);
    }
    return layers;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Canvas
///
Canvas::Canvas(MainWindow* parent, const int width, const int height) :
    QTabWidget(),
    m_pParent(parent)
{
    CanvasLayer canvasLayer;
    canvasLayer.m_image = QImage(QSize(width, height), QImage::Format_ARGB32);
    canvasLayer.m_image.fill(Qt::transparent);
    m_canvasLayers.push_back(canvasLayer);

    init(width, height);
}

Canvas::Canvas(MainWindow *parent, QString& filePath, bool& loadSuccess) :
    QTabWidget(),
    m_pParent(parent)
{
    if(QFileInfo(filePath).suffix().contains(Constants::CanvasSaveFileType))
    {
        QFile inFile(filePath);
        inFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&inFile);

        while(!in.atEnd())
        {
            QString line = in.readLine();
            if(line == Constants::CanvasSaveLayerBegin)
            {
                CanvasLayer cl;

                //Read layer info
                cl.m_info.m_name = in.readLine();
                cl.m_info.m_enabled = in.readLine() == "1" ? true : false;

                //Read layer image data
                QByteArray ba;
                QTextStream(&ba) << in.readLine();
                QByteArray layerImageData = QByteArray::fromHex(ba);
                cl.m_image.loadFromData(layerImageData);

                if(cl.m_image != QImage() && !cl.m_image.isNull())
                {
                    m_canvasLayers.push_back(cl);
                }
            }
        }

        m_savePath = filePath;
    }
    else
    {
        QImage image(filePath);

        if(image != QImage() && !image.isNull())
        {
            CanvasLayer canvasLayer;
            canvasLayer.m_image = image;
            m_canvasLayers.push_back(canvasLayer);
        }
    }

    //Did we load successfully?
    if(m_canvasLayers.size() == 0)
    {
        loadSuccess = false;
        qDebug() << "Canvas::Canvas - Failed to load canvas!";
    }
    else
    {
        loadSuccess = true;
        init(m_canvasLayers[0].m_image.width(), m_canvasLayers[0].m_image.height());
    }
}

void Canvas::init(uint width, uint height)
{
    m_selectedLayer = 0;
    m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

    m_canvasWidth = width;
    m_canvasHeight = height;

    m_canvasBackgroundImage = genTransparentPixelsBackground(m_canvasWidth, m_canvasHeight);

    m_selectionTool = new QRubberBand(QRubberBand::Rectangle, this);
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    m_pClipboardPixels = new PaintableClipboard(this);
    m_pClipboardPixels->raise();

    m_pSelectedPixels = new SelectedPixels(this, m_canvasWidth, m_canvasHeight);
    m_pSelectedPixels->raise();

    m_canvasHistory.recordHistory(getSnapshot());

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
        textImage.fill(Qt::transparent);
        QPainter textPainter(&textImage);
        textPainter.setCompositionMode (QPainter::CompositionMode_Source);
        textPainter.setPen(m_pParent->getSelectedColor());
        textPainter.setFont(font);
        textPainter.drawText(m_textDrawLocation, m_textToDraw);

        m_pClipboardPixels->setImage(textImage);
        m_pSelectedPixels->clear();
        m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixels());

        m_canvasMutex.unlock();

        update();
    }
}

QString Canvas::getSavePath()
{
    return m_savePath;
}

bool Canvas::save(QString path)
{
    m_savePath = path;

    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    for(CanvasLayer& cl : m_canvasLayers)
    {
        out << Constants::CanvasSaveLayerBegin << "\n";;
        out << cl.m_info.m_name << "\n";
        out << cl.m_info.m_enabled << "\n";;

        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);

        cl.m_image.save(&buffer, "PNG");
        out << buffer.data().toHex() << "\n";
        out << Constants::CanvasSaveLayerEnd << "\n";
    }

    file.close();
    return true;
}

void Canvas::onLayerAdded()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    CanvasLayer canvasLayer;
    canvasLayer.m_image = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);
    canvasLayer.m_image.fill(Qt::transparent);
    m_canvasLayers.push_back(canvasLayer);

    m_canvasHistory.recordHistory(getSnapshot());
}

void Canvas::onLayerDeleted(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers.removeAt(index);
    m_canvasHistory.recordHistory(getSnapshot());
}

void Canvas::onLayerEnabledChanged(const uint index, const bool enabled)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers[index].m_info.m_enabled = enabled; //Assumes there is a layer at index
    m_canvasHistory.recordHistory(getSnapshot());
}

void Canvas::onLayerTextChanged(const uint index, QString text)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_canvasLayers[index].m_info.m_name = text; //Assumes there is a layer at index
    m_canvasHistory.recordHistory(getSnapshot());
}

void Canvas::onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(layerIndexA < (uint)m_canvasLayers.count() && layerIndexB < (uint)m_canvasLayers.count() && m_selectedLayer == layerIndexA)
    {
        //Paint layer b onto layer a
        QPainter mergePainter(&m_canvasLayers[layerIndexA].m_image);
        mergePainter.setCompositionMode (QPainter::CompositionMode_SourceAtop);
        mergePainter.drawImage(m_canvasLayers[layerIndexB].m_image.rect(), m_canvasLayers[layerIndexB].m_image);
        mergePainter.end();

        //Combine names of layers
        m_canvasLayers[layerIndexA].m_info.m_name = m_canvasLayers[layerIndexA].m_info.m_name + " & " + m_canvasLayers[layerIndexB].m_info.m_name;

        //Remove layer b as now merged into layer a
        m_canvasLayers.removeAt(layerIndexB);

        //Update layer dialog on new layers
        m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

        m_canvasHistory.recordHistory(getSnapshot());

        update();
    }
    else
    {
        qDebug() << "Canvas::onLayerMergeRequested - layer indexes incorrect";
    }
}

void Canvas::onLayerMoveUp(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(index > 0 && (int)index < m_canvasLayers.size())
    {
        //Move up
        m_canvasLayers.swapItemsAt(index, index - 1);
        m_selectedLayer = index - 1;

        //Update layer dialog on new layers
        m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

        m_canvasHistory.recordHistory(getSnapshot());

        update();
    }
    else
    {
        qDebug() << "Canvas::onLayerMoveUp - incorrect index " << index;
    }
}

void Canvas::onLayerMoveDown(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if((int)index < m_canvasLayers.size() - 1)
    {
        //Move down
        m_canvasLayers.swapItemsAt(index + 1, index);
        m_selectedLayer = index + 1;

        //Update layer dialog on new layers
        m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

        m_canvasHistory.recordHistory(getSnapshot());

        update();
    }
    else
    {
        qDebug() << "Canvas::onLayerMoveDown - incorrect index " << index;
    }
}

void Canvas::onSelectedLayerChanged(const uint index)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Reset effects incase in the middle of effects when switched layer
    if(m_beforeEffectsImage != QImage())
    {
        canvasMutexLocker.unlock();
        onCancelEffects();
        canvasMutexLocker.relock();
    }

    m_selectedLayer = index;
}

void Canvas::onLoadLayer(CanvasLayer canvasLayer)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    //Take canvasLayer's image and map it to an image with m_canvasWidth, m_canvasHeight dimensions
    QImage newLayerImage = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);
    newLayerImage.fill(Qt::transparent);
    QPainter painter(&newLayerImage);
    painter.drawImage(newLayerImage.rect(), canvasLayer.m_image, canvasLayer.m_image.rect());
    painter.end();

    //Use image with correct dimensions as one to add
    canvasLayer.m_image = newLayerImage;

    //Add layer
    m_canvasLayers.push_back(canvasLayer);

    //Update layers dlg
    m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

    m_canvasHistory.recordHistory(getSnapshot());
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
            m_canvasHistory.recordHistory(getSnapshot());
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
            if(m_pSelectedPixels->containsPixels())
            {
                //Clear selected pixels
                m_pSelectedPixels->clear();

                m_canvasHistory.recordHistory(getSnapshot());
            }
        }

        canvasMutexLocker.unlock();

        doUpdate = true;
    }

    if(m_tool == TOOL_DRAG && m_pSelectedPixels->containsPixels())
    {
        m_pClipboardPixels->generateClipboard(m_canvasLayers[m_selectedLayer].m_image, m_pSelectedPixels);
    }

    if (doUpdate)
        update();
}

void Canvas::onDeleteKeyPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    if(m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, Qt::transparent);//Assumes there is a selected layer
        });
        m_pSelectedPixels->clear();

        m_canvasHistory.recordHistory(getSnapshot());

        canvasMutexLocker.unlock();

        update();
    }
    else if(m_tool == TOOL_DRAG)
    {
        m_pClipboardPixels->reset();
        m_pSelectedPixels->clear();
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
        clipBoard.m_clipboardImage.fill(Qt::transparent);

        //Go through selected pixels cutting from canvas and copying to clipboard
        m_pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            clipBoard.m_clipboardImage.setPixelColor(x, y, m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y));//Assumes there is a selected layer
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, Qt::transparent);//Assumes there is a selected layer
            clipBoard.m_pixels.push_back(QPoint(x,y));
        });
    }

    //Reset
    m_pClipboardPixels->reset();
    m_pSelectedPixels->clear();
    m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

    m_canvasHistory.recordHistory(getSnapshot());

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

    CanvasHistoryItem snapShot;
    if(m_canvasHistory.undoHistory(snapShot))
    {
        m_canvasLayers = snapShot.m_layers;
        m_pClipboardPixels->setClipboard(snapShot.m_clipboard);
        m_pSelectedPixels->clear();
        m_pSelectedPixels->addPixels(snapShot.m_selectedPixels);

        //Incase m_selectedLayer is now out of bounds due to m_canvasLayers changing
        if((int)m_selectedLayer >= m_canvasLayers.size())
        {
            m_selectedLayer = m_canvasLayers.size() - 1; //assumes theres at least one layer
        }

        m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

        update();
    }
}

void Canvas::onRedoPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    CanvasHistoryItem snapShot;
    if(m_canvasHistory.redoHistory(snapShot))
    {
        m_canvasLayers = snapShot.m_layers;
        m_pClipboardPixels->setClipboard(snapShot.m_clipboard);
        m_pSelectedPixels->clear();
        m_pSelectedPixels->addPixels(snapShot.m_selectedPixels);

        m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

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

    m_canvasHistory.recordHistory(getSnapshot());

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

    m_canvasHistory.recordHistory(getSnapshot());

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

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

    //Record history is done in onConfirmEffects()

    update();
}

void Canvas::onConfirmEffects()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_beforeEffectsImage = QImage();
    m_canvasHistory.recordHistory(getSnapshot());
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
    return m_canvasLayers[m_selectedLayer].m_image;
}

float Canvas::getZoom()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return m_zoomFactor;
}

QPoint Canvas::getPanOffset()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    return QPoint(m_panOffsetX, m_panOffsetY);
}

CanvasHistoryItem Canvas::getSnapshot()
{
    CanvasHistoryItem canvasHistoryItem;
    canvasHistoryItem.m_layers = m_canvasLayers;
    canvasHistoryItem.m_clipboard = m_pClipboardPixels->getClipboard();
    canvasHistoryItem.m_selectedPixels = m_pSelectedPixels->getPixels();
    return canvasHistoryItem;
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

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    Q_UNUSED(paintEvent);

    m_canvasMutex.lock();

    //Setup painter
    QPainter painter(this);

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
            m_zoomFactor *= (Constants::ZoomIncrement);
    }
    else if(event->angleDelta().y() < 0)
    {
        if(m_zoomFactor > 0.1)
            m_zoomFactor /= (Constants::ZoomIncrement);
    }

    //Call to redraw
    update();
}

void Canvas::onParentMouseScroll(QWheelEvent* event)
{
    wheelEvent(event);
}

void Canvas::showEvent(QShowEvent *)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    m_pParent->setLayers(getLayerInfoList(m_canvasLayers), m_selectedLayer);

    canvasMutexLocker.unlock();

    emit canvasSizeChange(m_canvasWidth, m_canvasHeight);
    emit selectionAreaResize(0,0);
}

QPointF getPositionRelativeCenterdAndZoomedCanvas(QPointF globalPos, QPoint& center, const float& zoomFactor, const int& offsetX, const int& offsetY)
{
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.scale(zoomFactor, zoomFactor);
    transform.translate(-center.x(), -center.y());
    const QPointF zoomPoint = transform.inverted().map(QPointF(globalPos.x(), globalPos.y()));
    return QPointF(zoomPoint.x() - offsetX, zoomPoint.y() - offsetY);
}

QPoint getPositionRelativeCenterdAndZoomedCanvas(QPoint globalPos, QPoint& center, const float& zoomFactor, const int& offsetX, const int& offsetY)
{
    QPointF pos = getPositionRelativeCenterdAndZoomedCanvas(QPointF(globalPos), center, zoomFactor, offsetX, offsetY);
    return pos.toPoint();
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
                image.setPixelColor(x, y, newColor);

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

    //If were not dragging, and the clipboard shows something. Dump it - unless editing text
    if(m_tool != TOOL_DRAG && !m_pClipboardPixels->isImageDefault() && m_tool != TOOL_TEXT)
    {
        //Dump clipboard, if something actually dumped record image history
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);
        if(m_pClipboardPixels->dumpImage(painter))
        {
            m_canvasHistory.recordHistory(getSnapshot());
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

        m_canvasHistory.recordHistory(getSnapshot());

        update();
    }
    else if(m_tool == TOOL_BUCKET)
    {
        floodFillOnSimilar(m_canvasLayers[m_selectedLayer].m_image, m_pParent->getSelectedColor(), mouseLocation.x(), mouseLocation.y(), m_pParent->getSpreadSensitivity());

        m_canvasHistory.recordHistory(getSnapshot());

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
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);//assumes theres always a selected layer
        m_pClipboardPixels->dumpImage(painter);
        painter.end();

        update();

        m_canvasHistory.recordHistory(getSnapshot());

        m_pSelectedPixels->clear();

        m_drawShapeOrigin = mouseLocation;
    }
    else if(m_tool == TOOL_DRAG)
    {
        if(m_pClipboardPixels->nubblesDrag(getPositionRelativeCenterdAndZoomedCanvas(mouseEvent->localPos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY), m_zoomFactor))
        {
            //Clear selected pixels and set to clipboard pixels
            m_pSelectedPixels->clear();
            m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixelsOffset());
        }
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    Q_UNUSED(releaseEvent);

    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_bMouseDown = false;

    if(m_tool == TOOL_SELECT)
    {
        m_pSelectedPixels->addPixels(m_selectionTool);
        m_canvasHistory.recordHistory(getSnapshot());
        update();
    }
    else if(m_tool == TOOL_PAN)
    {
        m_previousPanPos = m_c_nullPanPos;
    }
    else if (m_tool == TOOL_PAINT || m_tool == TOOL_ERASER)
    {
        m_canvasHistory.recordHistory(getSnapshot());
    }
    else if(m_tool == TOOL_SHAPE)
    {
        m_pSelectedPixels->clear();
        m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixels());

        update();
    }
    else if(m_tool == TOOL_DRAG)
    {
        if(m_pClipboardPixels->completeOperation())
        {
            m_canvasHistory.recordHistory(getSnapshot());
        }
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
            const bool isDragging = m_pClipboardPixels->isDragging();
            if(!isDragging && m_pClipboardPixels->nubblesDrag(getPositionRelativeCenterdAndZoomedCanvas(event->localPos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY), m_zoomFactor))
            {
                //Clear selected pixels and set to clipboard pixels
                m_pSelectedPixels->clear();
                m_pSelectedPixels->addPixels(m_pClipboardPixels->getPixelsOffset());
            }
            else
            {
                //If starting dragging
                if(!isDragging)
                {
                    //check if mouse is over selection area
                    if(m_pSelectedPixels->isHighlighted(mouseLocation.x(), mouseLocation.y()))
                    {
                        //If no clipboard exists to drag, generate one based on selected pixels
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
        }
        else if(m_tool == TOOL_SHAPE)
        {
            m_pClipboardPixels->reset();

            QImage newShapeImage = QImage(QSize(m_canvasWidth, m_canvasHeight), QImage::Format_ARGB32);
            newShapeImage.fill(Qt::transparent);

            QPainter shapePainter(&newShapeImage);

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

QList<QPoint> SelectedPixels::getPixels()
{
    return m_selectedPixelsList;
}

void SelectedPixels::paintEvent(QPaintEvent *paintEvent)
{
    Q_UNUSED(paintEvent);

    QPainter painter(this);

    const QPoint center = QPoint(geometry().width() / 2, geometry().height() / 2);
    painter.translate(center);
    painter.scale(m_pParentCanvas->getZoom(), m_pParentCanvas->getZoom());
    painter.translate(-center);

    //Offsets
    QPoint offset = m_pParentCanvas->getPanOffset();

    //Outline
    m_bOutlineColorToggle = !m_bOutlineColorToggle;
    QPen selectionOutlinePen = QPen(m_bOutlineColorToggle ? Qt::black : Qt::white, 1/m_pParentCanvas->getZoom());
    painter.setPen(selectionOutlinePen);

    //Paint pixels and outline
    for(QPoint p : m_selectedPixelsList)
    {
        const uint x = p.x();
        const uint y = p.y();

        painter.fillRect(QRect(x + offset.x(), y + offset.y(), 1, 1), Constants::SelectionAreaColor);

        //border right
        if(x == m_selectedPixels.size()-1 || (x + 1 < m_selectedPixels.size() && !m_selectedPixels[x+1][y]))
        {
            painter.drawLine(QPoint(x + offset.x() + 1, y + offset.y()), QPoint(x + offset.x() + 1, y + offset.y() + 1));
        }

        //border left
        if(x == 0 || (x > 0 && !m_selectedPixels[x-1][y]))
        {
            painter.drawLine(QPoint(x + offset.x(), y + offset.y()), QPoint(x + offset.x(), y + offset.y() + 1));
        }

        //border bottom
        if(y == m_selectedPixels[x].size()-1 || (y + 1 < m_selectedPixels.size() && !m_selectedPixels[x][y+1]))
        {
            painter.drawLine(QPoint(x + offset.x(), y + offset.y() + 1.0), QPoint(x + offset.x() + 1.0, y + offset.y() + 1.0));
        }

        //border top
        if(y == 0 || (y > 0 && !m_selectedPixels[x][y-1]))
        {
            painter.drawLine(QPoint(x + offset.x(), y + offset.y()), QPoint(x + offset.x() + 1.0, y + offset.y()));
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
    m_clipboardImage.fill(Qt::transparent);

    m_pixels.clear();

    pSelectedPixels->operateOnSelectedPixels([&](int x, int y)-> void
    {
        m_clipboardImage.setPixelColor(x, y, canvas.pixelColor(x,y));
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

    m_dragNubbles.insert(DragNubblePos::TopLeft, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() < dimensions.right() && mouseLocation.y() < dimensions.bottom())
        {
             dimensions.setX(mouseLocation.x());
             dimensions.setY(mouseLocation.y());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::TopMiddle, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.y() < dimensions.bottom())
        {
            dimensions.setY(mouseLocation.y());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::TopRight, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() > dimensions.left() && mouseLocation.y() < dimensions.bottom())
        {
            dimensions.setRight(mouseLocation.x());
            dimensions.setY(mouseLocation.y());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::LeftMiddle, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() < dimensions.right())
        {
            dimensions.setX(mouseLocation.x());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::RightMiddle, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() > dimensions.left())
        {
            dimensions.setRight(mouseLocation.x());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::BottomLeft, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() < dimensions.right() && mouseLocation.y() > dimensions.top())
        {
            dimensions.setX(mouseLocation.x());
            dimensions.setBottom(mouseLocation.y());
        }

    }));
    m_dragNubbles.insert(DragNubblePos::BottomMiddle, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.y() > dimensions.top())
        {
            dimensions.setBottom(mouseLocation.y());
        }
    }));
    m_dragNubbles.insert(DragNubblePos::BottomRight, DragNubble([&](QRect& dimensions, const QPointF& mouseLocation)-> void
    {
        if(mouseLocation.x() > dimensions.left() && mouseLocation.y() > dimensions.top())
        {
            dimensions.setRight(mouseLocation.x());
            dimensions.setBottom(mouseLocation.y());
        }
    }));
}

void PaintableClipboard::generateClipboard(QImage &canvas, SelectedPixels *pSelectedPixels)
{
    Clipboard::generateClipboard(canvas, pSelectedPixels);
    updateDimensionsRect();
    update();
}

void PaintableClipboard::setClipboard(Clipboard clipboard)
{
    m_clipboardImage = clipboard.m_clipboardImage;
    m_pixels = clipboard.m_pixels;
    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = 0;
    m_dragY = 0;
    updateDimensionsRect();
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

    updateDimensionsRect();

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
    for(QPoint& p : m_pixels)
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
    m_dimensionsRect = QRect();
}

void scaleImageOntoSelf(QImage& imageToScale, QRect oldDimensions, QRect newDimensions)
{
    QImage oldImage = imageToScale;

    imageToScale.fill(Qt::transparent);
    QPainter clipboardPainter(&imageToScale);
    clipboardPainter.drawImage(newDimensions, oldImage, oldDimensions);
}

QPointF getLocation(QRectF rect, DragNubblePos nubblePos)
{
    switch (nubblePos)
    {
        case DragNubblePos::TopLeft:
            return rect.topLeft();
        case DragNubblePos::TopRight:
            return rect.topRight();
        case DragNubblePos::BottomLeft:
            return rect.bottomLeft();
        case DragNubblePos::BottomRight:
            return rect.bottomRight();
        case DragNubblePos::TopMiddle:
            return QPointF(rect.center().x(), rect.topLeft().y());
        case DragNubblePos::BottomMiddle:
            return QPointF(rect.center().x(), rect.bottomLeft().y());
        case DragNubblePos::LeftMiddle:
            return QPointF(rect.topLeft().x(), rect.center().y());
        case DragNubblePos::RightMiddle:
            return QPointF(rect.topRight().x(), rect.center().y());
    }

    qDebug() << "getLocation(QRectF rect, DragNubblePos nubblePos) : nubble pos not found!";
    return rect.topLeft();
}

bool PaintableClipboard::nubblesDrag(QPointF mouseLocation, const float& zoom)
{

    //If one of the nubbles is already dragging, continue dragging
    for(const auto& nubblePos : m_dragNubbles.keys())
    {
        if(m_dragNubbles[nubblePos].isDragging())
        {
            m_dragNubbles[nubblePos].doDragging(mouseLocation, m_dimensionsRect);
            doNubbleDragScale();
            return true;
        }
    }

    //Check if a nubble is being selected
    for(const auto& nubblePos : m_dragNubbles.keys())
    {
        if(m_dragNubbles[nubblePos].isStartDragging(mouseLocation, getLocation(m_dimensionsRect, nubblePos), zoom))
        {
            prepNubblesDrag();
            return true;
        }
    }

    return false;
}

void PaintableClipboard::paintEvent(QPaintEvent *paintEvent)
{
    Q_UNUSED(paintEvent);

    QPainter painter(this);

    const QPoint center = QPoint(geometry().width() / 2, geometry().height() / 2);
    const float zoom = m_pParentCanvas->getZoom();
    painter.translate(center);
    painter.scale(zoom, zoom);
    painter.translate(-center);

    QPoint offset = m_pParentCanvas->getPanOffset();
    const int offsetX = offset.x() + m_dragX;
    const int offsetY = offset.y() + m_dragY;

    //Draw clipboard
    painter.drawImage(QRect(offsetX, offsetY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);

    //Draw transparent selected pixels ~ todo - So inneficient! look for something else
    for(QPoint& p : m_pixels)
    {
        if(m_clipboardImage.pixelColor(p.x(), p.y()).alpha() == 0)
        {
            const QColor col = (p.x() % 2 == 0) ?
                        (p.y() % 2 == 0) ? Constants::TransparentWhite : Constants::TransparentGrey
                                     :
                        (p.y() % 2 == 0) ? Constants::TransparentGrey : Constants::TransparentWhite;

            painter.fillRect(QRect(p.x() + offsetX, p.y() + offsetY, 1, 1), col);
        }
    }

    //Draw nubbles to scale dimension of clipboard
    if(m_pixels.size() > 0)
    {
        QRectF translatedDimensions = m_dimensionsRect.translated((offsetX), (offsetY));

        for(const auto& nubblePos : m_dragNubbles.keys())
        {
            m_dragNubbles[nubblePos].draw(painter, zoom, getLocation(translatedDimensions, nubblePos));
        }
    }
}

void PaintableClipboard::updateDimensionsRect()
{
    m_dimensionsRect = QRect();
    for(QPoint& p : m_pixels)
    {
        if(m_dimensionsRect.x() == 0 || m_dimensionsRect.x() > p.x())
        {
            m_dimensionsRect.setX(p.x());
        }
        if(m_dimensionsRect.y() == 0 || m_dimensionsRect.y() > p.y())
        {
            m_dimensionsRect.setY(p.y());
        }
        if(m_dimensionsRect.right() < p.x())
        {
            m_dimensionsRect.setRight(p.x());
        }
        if(m_dimensionsRect.bottom() < p.y())
        {
            m_dimensionsRect.setBottom(p.y());
        }
    }
}

bool PaintableClipboard::completeOperation()
{
    if(isDragging())
    {
        completeNormalDrag();
        return true;
    }

    for(const auto& nubblePos : m_dragNubbles.keys())
    {
        if(m_dragNubbles[nubblePos].isDragging())
        {
            m_dragNubbles[nubblePos].setDragging(false);
            completeNubbleDrag();
            return true;
        }
    }

    return false;
}

void PaintableClipboard::completeNormalDrag()
{
    //Create new clipboard image
    QImage newClipboardImage = QImage(QSize(m_clipboardImage.width(), m_clipboardImage.height()), QImage::Format_ARGB32);
    newClipboardImage.fill(Qt::transparent);

    //Dump nubble changed (re-scaled) current clipboard image onto new one
    QPainter clipPainter(&newClipboardImage);
    clipPainter.drawImage(QRect(m_dragX, m_dragY, m_clipboardImage.width(), m_clipboardImage.height()), m_clipboardImage);
    clipPainter.end();

    //Set new values & reset & redraw
    m_clipboardImage = newClipboardImage;
    m_pixels = getPixelsOffset();

    //Temp bug fix. Later I want to make it so you can drag off canvas and then drag back on....
    for(int i = 0; i < m_pixels.size(); i++)
    {
        if(m_pixels[i].x() < 0 || m_pixels[i].x() > m_clipboardImage.width() || m_pixels[i].y() < 0 || m_pixels[i].y() > m_clipboardImage.height())
        {
            m_pixels.removeAt(i);
            i--;
        }
    }

    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = 0;
    m_dragY = 0;
    updateDimensionsRect();
    update();
}

void PaintableClipboard::doNubbleDragScale()
{
    m_clipboardImage = m_clipboardImageBeforeNubbleDrag;

    //Scale
    scaleImageOntoSelf(m_clipboardImage, m_dimensionsRectBeforeNubbleDrag, m_dimensionsRect);

    //Scale selected transparent pixels (cheat by converting to black)
    QImage m_clipboardImageTransparent = m_clipboardImageBeforeNubbleDragTransparent;
    scaleImageOntoSelf(m_clipboardImageTransparent, m_dimensionsRectBeforeNubbleDrag, m_dimensionsRect);

    //Set new pixels based off scaled image
    m_pixels.clear();
    for(int x = 0; x < m_clipboardImage.width(); x++)
    {
        for(int y = 0; y < m_clipboardImage.height(); y++)
        {
            if(m_clipboardImage.pixelColor(x,y).alpha() > 0)
            {
                m_pixels.push_back(QPoint(x,y));
            }
            else if(m_clipboardImageTransparent.pixelColor(x,y).alpha() > 0)
            {
                m_pixels.push_back(QPoint(x,y));
            }
        }
    }

    updateDimensionsRect();
    update();
}

void PaintableClipboard::prepNubblesDrag()
{
    m_clipboardImageBeforeNubbleDrag = m_clipboardImage;
    m_dimensionsRectBeforeNubbleDrag = m_dimensionsRect;

    m_clipboardImageBeforeNubbleDragTransparent = QImage(QSize(m_clipboardImageBeforeNubbleDrag.width(), m_clipboardImageBeforeNubbleDrag.height()), QImage::Format_ARGB32);
    m_clipboardImageBeforeNubbleDragTransparent.fill(Qt::transparent);
    for(QPoint& p : m_pixels)
    {
        if(m_clipboardImageBeforeNubbleDrag.pixelColor(p.x(), p.y()).alpha() == 0)
        {
            m_clipboardImageBeforeNubbleDragTransparent.setPixelColor(p.x(), p.y(), Qt::black);
        }
    }
}

void PaintableClipboard::completeNubbleDrag()
{
    //Create new clipboard image
    QImage newClipboardImage = QImage(QSize(m_clipboardImage.width(), m_clipboardImage.height()), QImage::Format_ARGB32);
    newClipboardImage.fill(Qt::transparent);

    //Dump nubble changed (re-scaled) current clipboard image onto new one
    QPainter clipPainter(&newClipboardImage);
    clipPainter.drawImage(m_clipboardImage.rect(), m_clipboardImage);
    clipPainter.end();

    //Set new values & reset & redraw
    m_clipboardImage = newClipboardImage;
    m_pixels = getPixelsOffset();
    updateDimensionsRect();
    update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CanvasHistory
///
void CanvasHistory::recordHistory(CanvasHistoryItem canvasSnapShot)
{
    m_history.push_back(canvasSnapShot);

    if((int)Constants::MaxCanvasHistory < m_history.size())
    {
        m_history.erase(m_history.begin());
    }

    m_historyIndex = m_history.size() - 1;
}

bool CanvasHistory::redoHistory(CanvasHistoryItem& canvasSnapShot)
{
    if((int)m_historyIndex < m_history.size() - 1)
    {
        canvasSnapShot = m_history[size_t(++m_historyIndex)];
        return true;
    }
    return false;
}

bool CanvasHistory::undoHistory(CanvasHistoryItem& canvasSnapShot)
{
    if(m_historyIndex > 0)
    {
        canvasSnapShot = m_history[size_t(--m_historyIndex)];
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DragNubble
///
DragNubble::DragNubble(std::function<void(QRect&, const QPointF&)> operation) :
    m_operation(operation)
{
    //Static m_image shared across all instances of DragNubble
    if(m_image.isNull())
    {
        m_image = QImage(QSize(Constants::DragNubbleSize, Constants::DragNubbleSize), QImage::Format_ARGB32);
        m_image.fill(Qt::black);
        QPainter nubbleImagePainter(&m_image);
        nubbleImagePainter.fillRect(QRect(1, 1, Constants::DragNubbleSize - 2, Constants::DragNubbleSize - 2), Qt::white);
    }
}

bool DragNubble::isDragging()
{
    return m_bIsDragging;
}

void DragNubble::setDragging(bool dragging)
{
    m_bIsDragging = dragging;
}

bool DragNubble::isStartDragging(const QPointF& mouseLocation, QPointF location, const float& zoom)
{
    const float nubbleSize = Constants::DragNubbleSize/zoom;
    const float halfNubbleSize = nubbleSize/2;
    if(mouseLocation.x() >= location.x() - halfNubbleSize && mouseLocation.x() <= location.x() + halfNubbleSize &&
       mouseLocation.y() >= location.y() - halfNubbleSize && mouseLocation.y() <= location.y() + halfNubbleSize)
    {
        m_bIsDragging = true;
        return true;
    }

    return false;
}

void DragNubble::doDragging(const QPointF &mouseLocation, QRect& rect)
{
    m_operation(rect, mouseLocation);
}

void DragNubble::draw(QPainter &painter, const float& zoom, const QPointF location)
{
    const float nubbleSize = Constants::DragNubbleSize / zoom;
    const float halfNubbleSize = nubbleSize/2;

    painter.drawImage(QRectF(location.x() - halfNubbleSize, location.y() - halfNubbleSize, nubbleSize, nubbleSize),
                      m_image,
                      m_image.rect());
}
