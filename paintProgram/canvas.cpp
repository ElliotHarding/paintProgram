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

//Bugs : saving clipboard before effects and saving clipboard for snapshot does not take into account the drag offsets of the clipboard.

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
const float ZoomPanFactor = 0.1;

//Dragging
const int DragNubbleSize = 8;
const QPoint NullDragPoint = QPoint(0,0);
}

QImage genTransparentPixelsBackground(const int width, const int height)
{
    // ~ Test if quicker to paint all white, then fill grey squares after
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

    m_canvasHistory.recordHistory(getSnapshot());

    setMouseTracking(true);
}

Canvas::~Canvas()
{
    if(m_selectionTool)
        delete m_selectionTool;
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

void Canvas::onUpdateText()
{
    m_canvasMutex.lock();

    QFontMetrics fontMetrics(m_pParent->getTextFont());

    const int textWidth = fontMetrics.width(m_textToDraw);
    const int textHeight = fontMetrics.height();

    QImage textImage = QImage(QSize(m_textDrawLocation.x() + textWidth, m_textDrawLocation.y() + textHeight), QImage::Format_ARGB32);
    textImage.fill(Qt::transparent);
    QPainter textPainter(&textImage);
    textPainter.setCompositionMode (QPainter::CompositionMode_Source);
    textPainter.setPen(m_pParent->getSelectedColor());
    textPainter.setFont(m_pParent->getTextFont());
    textPainter.drawText(m_textDrawLocation, m_textToDraw);

    m_pClipboardPixels->setImage(textImage);

    m_canvasMutex.unlock();
}

void Canvas::onWriteText(QString letter)
{
    m_canvasMutex.lock();

    if(letter == "\010")//backspace
    {
        m_textToDraw.chop(1);
    }
    else
    {
        m_textToDraw += letter;
    }

    m_canvasMutex.unlock();

    onUpdateText();
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
    if(m_beforeEffectsImage != QImage() || m_beforeEffectsClipboard.m_clipboardImage != QImage())
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

    for(CanvasLayer& canvasLayer : m_canvasLayers)
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
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(m_tool == TOOL_TEXT && t != TOOL_TEXT)
        m_textToDraw = "";

    m_tool = t;
}

void Canvas::onDeleteKeyPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->reset();
        m_canvasHistory.recordHistory(getSnapshot());
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, Qt::transparent);//Assumes there is a selected layer
        });

        m_pClipboardPixels->reset();

        m_canvasHistory.recordHistory(getSnapshot());

        canvasMutexLocker.unlock();

        update();
    }
}
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
void Canvas::onCopyKeysPressed()
{
    m_canvasMutex.lock();

    //IF were dragging
    if(m_pClipboardPixels->clipboardActive())
    {
        QGuiApplication::clipboard()->setImage(m_pClipboardPixels->m_clipboardImage);
    }

    //If were selecting
    else if(m_pClipboardPixels->containsPixels())
    {
        QImage clipboardImage = QImage(QSize(m_canvasLayers[m_selectedLayer].m_image.width(), m_canvasLayers[m_selectedLayer].m_image.height()), QImage::Format_ARGB32);
        clipboardImage.fill(Qt::transparent);

        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            clipboardImage.setPixelColor(x, y, m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y));
        });

        QGuiApplication::clipboard()->setImage(clipboardImage);
    }

    m_canvasMutex.unlock();
}

void Canvas::onCutKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    QImage clipboardImage;

    //What if already dragging something around?
    if(m_pClipboardPixels->clipboardActive())
    {
        clipboardImage = m_pClipboardPixels->getImage();

        //Reset
        m_pClipboardPixels->reset();

        m_canvasHistory.recordHistory(getSnapshot());
    }
    else
    {
        //Copy cut pixels to clipboard
        clipboardImage = QImage(QSize(m_canvasLayers[m_selectedLayer].m_image.width(),m_canvasLayers[m_selectedLayer].m_image.height()), QImage::Format_ARGB32);
        clipboardImage.fill(Qt::transparent);

        if(m_pClipboardPixels->containsPixels())
        {
            //Go through selected pixels cutting from canvas and copying to clipboard
            m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
            {
                clipboardImage.setPixelColor(x, y, m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y));//Assumes there is a selected layer
                m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, Qt::transparent);//Assumes there is a selected layer
            });

            //Reset
            m_pClipboardPixels->reset();

            m_canvasHistory.recordHistory(getSnapshot());
        }
    }

    QGuiApplication::clipboard()->setImage(clipboardImage);

    canvasMutexLocker.unlock();

    update();
}

void Canvas::onPasteKeysPressed()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(m_pClipboardPixels->clipboardActive())
    {
        //Dump clipboard
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);
        m_pClipboardPixels->dumpImage(painter);
        update();
    }

    QImage clipboardImage = QGuiApplication::clipboard()->image();
    m_pClipboardPixels->setImage(clipboardImage);

    m_canvasHistory.recordHistory(getSnapshot());

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

        //Incase m_selectedLayer is now out of bounds due to m_canvasLayers changing
        if((int)m_selectedLayer >= m_canvasLayers.size())
        {
            m_selectedLayer = m_canvasLayers.size() - 1; //assumes theres at least one layer - which there always is
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
    if(m_pClipboardPixels->clipboardActive())
    {
        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, greyScaleColor(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y)));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Loop through selected pixels, turning to white&black
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, greyScaleColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x, y)));
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
    if(m_pClipboardPixels->clipboardActive())
    {
        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, invertColor(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y)));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, invertColor(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y)));
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

bool checkCreateSketchOnPixel(QImage& original, QImage& sketch, const int& x, const int& y, const QColor sketchColor, const int& sensitivity)
{
    if(compareNeighbour(original, x, y, x+1, y, sensitivity))
    {
        sketch.setPixelColor(x, y, sketchColor);
    }
    else if(compareNeighbour(original, x, y, x-1, y, sensitivity))
    {
        sketch.setPixelColor(x, y, sketchColor);
    }
    else if(compareNeighbour(original, x, y, x, y+1, sensitivity))
    {
        sketch.setPixelColor(x, y, sketchColor);
    }
    else if(compareNeighbour(original, x, y, x, y-1, sensitivity))
    {
        sketch.setPixelColor(x, y, sketchColor);
    }
    else
    {
        return false;
    }
    return true;
}

void Canvas::onSketchEffect(const int sensitivity)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(sensitivity == 0)
    {
        update();
        return;
    }

    QImage inkSketch = QImage(QSize(m_canvasLayers[m_selectedLayer].m_image.width(), m_canvasLayers[m_selectedLayer].m_image.height()), QImage::Format_ARGB32);
    const QColor sketchColor = m_pParent->getSelectedColor() != Qt::white ? m_pParent->getSelectedColor() : Qt::black;

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        inkSketch.fill(Qt::transparent);

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {            
            if(!checkCreateSketchOnPixel(m_pClipboardPixels->m_clipboardImage, inkSketch, x, y, sketchColor, sensitivity))
            {
                inkSketch.setPixelColor(x, y, Qt::white);
            }
        });

        QPainter sketchPainter(&m_pClipboardPixels->m_clipboardImage);
        sketchPainter.drawImage(0,0,inkSketch);
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects(); //Assumes there is a selected layer

        inkSketch.fill(Qt::transparent);

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            if(!checkCreateSketchOnPixel(m_canvasLayers[m_selectedLayer].m_image, inkSketch, x, y, sketchColor, sensitivity))
            {
                inkSketch.setPixelColor(x, y, Qt::white);
            }
        });

        QPainter sketchPainter(&m_canvasLayers[m_selectedLayer].m_image);
        sketchPainter.drawImage(0,0,inkSketch);
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects(); //Assumes there is a selected layer

        inkSketch.fill(Qt::white);

        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            checkCreateSketchOnPixel(m_canvasLayers[m_selectedLayer].m_image, inkSketch, x, y, sketchColor, sensitivity);
        });

        m_canvasLayers[m_selectedLayer].m_image = inkSketch;
    }

    //Record history is done in onConfirmEffects()

    update();
}

void Canvas::onNormalBlur(const int value)
{
    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {

        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects(); //Assumes there is a selected layer

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {

        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects(); //Assumes there is a selected layer

        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {

        });
    }

    //Record history is done in onConfirmEffects()

    update();
}

void Canvas::onOutlineEffect(const int sensitivity)
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

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
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            checkCreateSketchOnPixel(m_pClipboardPixels->m_clipboardImage, outlineSketch, x, y, sketchColor, sensitivity);
        });

        //Dump outline sketch onto m_canvasImage
        QPainter sketchPainter(&m_pClipboardPixels->m_clipboardImage);
        sketchPainter.drawImage(0,0,outlineSketch);
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            checkCreateSketchOnPixel(m_canvasLayers[m_selectedLayer].m_image, outlineSketch, x, y, sketchColor, sensitivity);
        });

        //Dump outline sketch onto m_canvasImage
        QPainter sketchPainter(&m_canvasLayers[m_selectedLayer].m_image);
        sketchPainter.drawImage(0,0,outlineSketch);
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through pixels, if a border pixel set it to sketchColor
        operateOnCanvasPixels(m_canvasLayers[m_selectedLayer].m_image, [&](int x, int y)-> void
        {
            checkCreateSketchOnPixel(m_canvasLayers[m_selectedLayer].m_image, outlineSketch, x, y, sketchColor, sensitivity);
        });

        //Dump outline sketch onto m_canvasImage
        QPainter sketchPainter(&m_canvasLayers[m_selectedLayer].m_image);
        sketchPainter.drawImage(0,0,outlineSketch);
    }

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

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, changeBrightness(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y), value));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeBrightness(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

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

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, changeContrast(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y), value));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, changeContrast(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

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

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, limitRed(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y), value));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitRed(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

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

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, limitBlue(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y), value));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitBlue(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

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

    //check if were doing the whole image or just some selected pixels
    if(m_pClipboardPixels->clipboardActive())
    {
        m_pClipboardPixels->setClipboard(getClipboardBeforeEffects());

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_pClipboardPixels->m_clipboardImage.setPixelColor(x, y, limitGreen(m_pClipboardPixels->m_clipboardImage.pixelColor(x,y), value));
        });
    }
    else if(m_pClipboardPixels->containsPixels())
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

        //Loop through selected pixels
        m_pClipboardPixels->operateOnSelectedPixels([&](int x, int y)-> void
        {
            m_canvasLayers[m_selectedLayer].m_image.setPixelColor(x, y, limitGreen(m_canvasLayers[m_selectedLayer].m_image.pixelColor(x,y), value));
        });
    }
    else
    {
        //Get backup of canvas image before effects were applied (create backup if first effect)
        m_canvasLayers[m_selectedLayer].m_image = getCanvasImageBeforeEffects();

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
    m_beforeEffectsClipboard.m_clipboardImage = QImage();
    m_beforeEffectsClipboard.m_pixels.clear();
    m_canvasHistory.recordHistory(getSnapshot());
}

void Canvas::onCancelEffects()
{
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(m_beforeEffectsImage != QImage())
    {
        m_canvasLayers[m_selectedLayer].m_image = m_beforeEffectsImage;
        m_beforeEffectsImage = QImage();
    }
    else if(m_beforeEffectsClipboard.m_clipboardImage != QImage())
    {
        m_pClipboardPixels->setClipboard(m_beforeEffectsClipboard);
        m_beforeEffectsClipboard.m_clipboardImage = QImage();
        m_beforeEffectsClipboard.m_pixels.clear();
    }
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

//Requires m_canvasMutex to be locked!
CanvasHistoryItem Canvas::getSnapshot()
{
    CanvasHistoryItem canvasHistoryItem;
    canvasHistoryItem.m_layers = m_canvasLayers;
    canvasHistoryItem.m_clipboard = m_pClipboardPixels->getClipboard();
    return canvasHistoryItem;
}

void Canvas::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);

    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    updateCenter();

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
    m_panOffsetX -= xFromCenter * Constants::ZoomPanFactor * direction / m_zoomFactor;

    const int yFromCenter = event->position().y() - m_center.y();
    m_panOffsetY -= yFromCenter * Constants::ZoomPanFactor * direction / m_zoomFactor;

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

void spreadSelectSimilarColor(QImage& image, QVector<QVector<bool>>& selectedPixels, QPoint startPixel, int sensitivty)
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
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    if(mouseEvent->button() == Qt::MiddleButton)
    {
        m_bMiddleMouseDown = true;
        return;
    }

    m_bMouseDown = true;

    QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(mouseEvent->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);

    //If not dragging, and clipboard shows something. Dump it.
    // Unless : selecting and holding ctrl
    // Unless : writing text
    if(m_tool != TOOL_DRAG && m_pClipboardPixels->clipboardActive() &&
      !(m_pParent->isCtrlPressed() && (m_tool == TOOL_SELECT || m_tool == TOOL_SPREAD_ON_SIMILAR)) &&
       m_tool != TOOL_TEXT)
    {
        //Dump clipboard, if something actually dumped record image history
        QPainter painter(&m_canvasLayers[m_selectedLayer].m_image);
        if(m_pClipboardPixels->dumpImage(painter))
        {
            m_canvasHistory.recordHistory(getSnapshot());
        }
        update();
    }

    //If pixels are selected, and were not using selection tools. Loose the selection
    //  In the future may only wish todo operations like painting inside the selected pixels, so this is not permanant
    else if(m_pClipboardPixels->containsPixels() &&
            m_tool != TOOL_SELECT &&
            m_tool != TOOL_SPREAD_ON_SIMILAR &&
            m_tool != TOOL_DRAG)
    {
        m_pClipboardPixels->reset();
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
            m_pClipboardPixels->reset();
        }

        m_selectionToolOrigin = mouseLocation;
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));
    }
    else if(m_tool == TOOL_SPREAD_ON_SIMILAR)
    {
        if(!m_pParent->isCtrlPressed())
        {
            m_pClipboardPixels->reset();
        }

        QVector<QVector<bool>> newSelectedPixels = QVector<QVector<bool>>(m_canvasWidth, QVector<bool>(m_canvasHeight, false));
        spreadSelectSimilarColor(m_canvasLayers[m_selectedLayer].m_image, newSelectedPixels, mouseLocation, m_pParent->getSpreadSensitivity());
        m_pClipboardPixels->addPixels(m_canvasLayers[m_selectedLayer].m_image, newSelectedPixels);

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

        onUpdateText();

        canvasMutexLocker.relock();
    }
    else if(m_tool == TOOL_SHAPE)
    {
        m_drawShapeOrigin = mouseLocation;
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    Q_UNUSED(releaseEvent);

    QMutexLocker canvasMutexLocker(&m_canvasMutex);
    m_bMouseDown = false;

    if(m_bMiddleMouseDown)
    {
         m_bMiddleMouseDown = false;
         m_previousPanPos = m_c_nullPanPos;
         return;
    }

    if(m_tool == TOOL_SELECT)
    {
        m_pClipboardPixels->addPixels(m_canvasLayers[m_selectedLayer].m_image, m_selectionTool); //Assumes there is a selected layer
        m_canvasHistory.recordHistory(getSnapshot());

        //Reset selection rectangle tool
        m_selectionTool->setGeometry(QRect(m_selectionToolOrigin, QSize()));

        emit selectionAreaResize(0,0);

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
    else if(m_tool == TOOL_DRAG)
    {
        if(m_pClipboardPixels->checkFinishDragging())
        {
            m_canvasHistory.recordHistory(getSnapshot());
        }
    }
    else if(m_tool == TOOL_SHAPE)
    {
        if(m_pClipboardPixels->clipboardActive())
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
    QMutexLocker canvasMutexLocker(&m_canvasMutex);

    QPoint mouseLocation = getPositionRelativeCenterdAndZoomedCanvas(event->pos(), m_center, m_zoomFactor, m_panOffsetX, m_panOffsetY);
    emit mousePositionChange(mouseLocation.x(), mouseLocation.y());

    //If middle mouse down, ignore current tool & do pan operation
    if(m_bMiddleMouseDown)
    {
        if(m_previousPanPos == m_c_nullPanPos)
        {
            m_previousPanPos = event->pos();
        }
        else
        {
            m_panOffsetX += (event->pos().x() - m_previousPanPos.x())/m_zoomFactor;
            m_panOffsetY += (event->pos().y() - m_previousPanPos.y())/m_zoomFactor;

            m_previousPanPos = event->pos();

            update();
        }
        return;
    }

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
                m_previousPanPos = event->pos();
            }
            else
            {
                m_panOffsetX += (event->pos().x() - m_previousPanPos.x())/m_zoomFactor;
                m_panOffsetY += (event->pos().y() - m_previousPanPos.y())/m_zoomFactor;

                m_previousPanPos = event->pos();

                update();
            }
        }
        else if(m_tool == TOOL_DRAG)
        {
            m_pClipboardPixels->checkDragging(m_canvasLayers[m_selectedLayer].m_image, mouseLocation, event->localPos(), m_zoomFactor, m_panOffsetX, m_panOffsetY);
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

//Requires m_canvasMutex to be locked!
Clipboard Canvas::getClipboardBeforeEffects()
{
    if(m_beforeEffectsClipboard.m_clipboardImage == QImage())
    {
        m_beforeEffectsClipboard = m_pClipboardPixels->getClipboard();
    }
    return m_beforeEffectsClipboard;
}

void Canvas::updateCenter()
{
    m_center = QPoint(geometry().width() / 2, geometry().height() / 2);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PaintableClipboard
///
PaintableClipboard::PaintableClipboard(Canvas* parent) : QWidget(parent),
    m_previousDragPos(Constants::NullDragPoint),
    m_pParentCanvas(parent)
{
    setGeometry(0, 0, parent->width(), parent->height());

    m_pOutlineDrawTimer = new QTimer(this);
    connect(m_pOutlineDrawTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_pOutlineDrawTimer->start(Constants::SelectedPixelsOutlineFlashFrequency);

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

PaintableClipboard::~PaintableClipboard()
{
    if(m_pOutlineDrawTimer)
        delete m_pOutlineDrawTimer;
}

void PaintableClipboard::generateClipboard(QImage &canvas)
{
    //Prep selected pixels for dragging
    m_clipboardImage = QImage(QSize(canvas.width(), canvas.height()), QImage::Format_ARGB32);
    m_clipboardImage.fill(Qt::transparent);

    for(const QPoint& p: m_pixels)
    {
        m_clipboardImage.setPixelColor(p.x(), p.y(), canvas.pixelColor(p.x(),p.y()));
    }

    m_backgroundImage = genTransparentPixelsBackground(m_clipboardImage.width(), m_clipboardImage.height());
    updateDimensionsRect();
    update();
}

void PaintableClipboard::setClipboard(Clipboard clipboard)
{
    m_clipboardImage = clipboard.m_clipboardImage;
    m_backgroundImage = genTransparentPixelsBackground(m_clipboardImage.width(), m_clipboardImage.height());
    m_pixels = clipboard.m_pixels;
    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = clipboard.m_dragX;
    m_dragY = clipboard.m_dragY;
    updateDimensionsRect();
    update();
}

Clipboard PaintableClipboard::getClipboard()
{
    Clipboard clipboard;
    clipboard.m_clipboardImage = m_clipboardImage;
    clipboard.m_pixels = m_pixels;
    clipboard.m_dragX = m_dragX;
    clipboard.m_dragY = m_dragY;
    return clipboard;
}

bool PaintableClipboard::clipboardActive()
{
    return m_clipboardImage != QImage();
}

void PaintableClipboard::setImage(QImage& image)
{
    m_clipboardImage = image;
    m_backgroundImage = genTransparentPixelsBackground(m_clipboardImage.width(), m_clipboardImage.height());

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

QImage& PaintableClipboard::getImage()
{
    return m_clipboardImage;
}

//Returns false if no image dumped
bool PaintableClipboard::dumpImage(QPainter &painter)
{
    if(m_clipboardImage == QImage())
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
    return true;
}

bool PaintableClipboard::isHighlighted(const int& x, const int& y)
{
    for(QPoint& p : m_pixels)
    {
        if(p.x() + m_dragX == x && p.y() + m_dragY == y)
        {
            return true;
        }
    }
    return false;
}

bool PaintableClipboard::containsPixels()
{
    return m_pixels.size() > 0;
}

QVector<QPoint> PaintableClipboard::getPixels()
{
    return m_pixels;
}

QVector<QPoint> PaintableClipboard::getPixelsOffset()
{
    QVector<QPoint> pixels = m_pixels;
    for(QPoint& p : pixels)
    {
        p.setX(p.x() + m_dragX);
        p.setY(p.y() + m_dragY);
    }
    return pixels;
}

void PaintableClipboard::operateOnSelectedPixels(std::function<void (int, int)> func)
{
    for(QPoint& p : m_pixels)
    {
        func(p.x(), p.y());
    }
}

QRect getPixelsDimensions(QVector<QPoint>& pixels)
{
    QRect returnRect = QRect();
    for(QPoint& p : pixels)
    {
        if(returnRect.x() == 0 || returnRect.x() > p.x())
        {
            returnRect.setX(p.x());
        }
        if(returnRect.y() == 0 || returnRect.y() > p.y())
        {
            returnRect.setY(p.y());
        }
        if(returnRect.right() < p.x())
        {
            returnRect.setRight(p.x());
        }
        if(returnRect.bottom() < p.y())
        {
            returnRect.setBottom(p.y());
        }
    }

    return returnRect;
}

void PaintableClipboard::addImageToActiveClipboard(QImage& newPixelsImage)
{
    //Save old m_drags
    int oldDragX = m_dragX;
    int oldDragY = m_dragY;

    //Get new m_drags
    QRect dimensionsRect = getPixelsDimensions(m_pixels);
    m_dragX = dimensionsRect.left() < 0 ? dimensionsRect.left() : 0;
    m_dragY = dimensionsRect.top() < 0 ? dimensionsRect.top() : 0;

    //Offset pixels based on new m_drags
    if(m_dragX != 0 || m_dragY != 0)
    {
        for(QPoint& p : m_pixels)
        {
            p.setX(p.x() - m_dragX);
            p.setY(p.y() - m_dragY);
        }
    }

    //Create new clipboard image
    const int newClipboardWidth = dimensionsRect.right() - m_dragX;
    const int newClipboardHeight = dimensionsRect.bottom() - m_dragY;
    QImage newClipboardImage = QImage(QSize(newClipboardWidth, newClipboardHeight), QImage::Format_ARGB32);
    newClipboardImage.fill(Qt::transparent);

    //Paint and combine new pixels and old clipboard onto new clipboard
    QPainter painter(&newClipboardImage);
    painter.drawImage(newPixelsImage.rect().translated(-m_dragX, -m_dragY), newPixelsImage, newPixelsImage.rect());
    painter.drawImage(m_clipboardImage.rect().translated(oldDragX - m_dragX, oldDragY - m_dragY), m_clipboardImage, m_clipboardImage.rect());
    painter.end();

    //Set new clipboard
    m_clipboardImage = newClipboardImage;
    m_backgroundImage = genTransparentPixelsBackground(m_clipboardImage.width(), m_clipboardImage.height());

    updateDimensionsRect();
    update();
}

void PaintableClipboard::addPixels(QImage& canvas, QRubberBand* newSelectionArea)
{
    if(newSelectionArea == nullptr)
        return;

    //Get selection area geometry and limit it to valid canvas locations
    const QRect geometry = newSelectionArea->geometry();
    const int selectionLeft = geometry.x() >= 0 ? geometry.x() : 0;
    const int selectionRight = geometry.right() <= canvas.width() ? geometry.right() : canvas.width();
    const int selectionTop = geometry.top() >= 0 ? geometry.top() : 0;
    const int selectionBottom = geometry.bottom() <= canvas.height() ? geometry.bottom() : canvas.height();

    //If theres no active clipboard (no dragging or re-shaping has been done) then just add pixels
    if(!clipboardActive())
    {
        for (int x = selectionLeft; x < selectionRight; x++)
        {
            for (int y = selectionTop; y < selectionBottom; y++)
            {
                m_pixels.push_back(QPoint(x,y));
            }
        }

        //Remove duplicates
        m_pixels.erase(std::unique(m_pixels.begin(), m_pixels.end()), m_pixels.end());

        updateDimensionsRect();
        update();

        return;
    }

    //Otherwise - create image from new pixels to add to existing clipboard:

    QImage newPixelsImage = QImage(QSize(canvas.width(), canvas.height()), QImage::Format_ARGB32);
    newPixelsImage.fill(Qt::transparent);

    //Gather all pixels in position relative to parent canvas
    m_pixels = getPixelsOffset();
    for (int x = selectionLeft; x < selectionRight; x++)
    {
        for (int y = selectionTop; y < selectionBottom; y++)
        {
            m_pixels.push_back(QPoint(x,y));
            newPixelsImage.setPixelColor(x, y, canvas.pixelColor(x,y));
        }
    }

    //Remove duplicates
    m_pixels.erase(std::unique(m_pixels.begin(), m_pixels.end()), m_pixels.end());

    addImageToActiveClipboard(newPixelsImage);
}

void PaintableClipboard::addPixels(QImage& canvas, QVector<QVector<bool>>& selectedPixels)
{
    //If theres no active clipboard (no dragging or re-shaping has been done) then just add pixels
    if(!clipboardActive())
    {
        for(int x = 0; x < selectedPixels.size(); x++)
        {
            for(int y = 0; y < selectedPixels[x].size(); y++)
            {
                if(selectedPixels[x][y])
                {
                    m_pixels.push_back(QPoint(x,y));
                }
            }
        }

        //Remove duplicates
        m_pixels.erase(std::unique(m_pixels.begin(), m_pixels.end()), m_pixels.end());

        updateDimensionsRect();
        update();

        return;
    }

    //Otherwise - create image from new pixels to add to existing clipboard:

    QImage newPixelsImage = QImage(QSize(canvas.width(), canvas.height()), QImage::Format_ARGB32);
    newPixelsImage.fill(Qt::transparent);

    //Gather all pixels in position relative to parent canvas
    m_pixels = getPixelsOffset();

    for(int x = 0; x < selectedPixels.size(); x++)
    {
        for(int y = 0; y < selectedPixels[x].size(); y++)
        {
            if(selectedPixels[x][y])
            {
                m_pixels.push_back(QPoint(x,y));
                newPixelsImage.setPixelColor(x, y, canvas.pixelColor(x,y));
            }
        }
    }

    //Remove duplicates
    m_pixels.erase(std::unique(m_pixels.begin(), m_pixels.end()), m_pixels.end());

    addImageToActiveClipboard(newPixelsImage);
}

void PaintableClipboard::checkDragging(QImage &canvasImage, QPoint mouseLocation, QPointF globalMouseLocation, const float &zoom, const int &panOffsetX, const int &panOffsetY)
{
    if(!isNormalDragging())
    {
        if(!doNubblesDrag(canvasImage, globalMouseLocation, zoom, panOffsetX, panOffsetY))
        {
            //check if mouse is over selection area
            if(isHighlighted(mouseLocation.x(), mouseLocation.y()))
            {
                //If no clipboard exists to drag, generate one based on selected pixels
                if(!clipboardActive())
                {
                    generateClipboard(canvasImage);
                }

                startNormalDragging(mouseLocation);
            }
        }
    }
    else
    {
        doNormalDragging(mouseLocation);
    }
}

bool PaintableClipboard::checkFinishDragging()
{
    if(isNormalDragging())
    {
        completeNormalDrag();
        return true;
    }

    const QList nubbleKeys = m_dragNubbles.keys();
    for(const auto& nubblePos : nubbleKeys)
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

bool PaintableClipboard::isNormalDragging()
{
    return (m_previousDragPos != Constants::NullDragPoint);
}

void PaintableClipboard::startNormalDragging(QPoint mouseLocation)
{
    m_previousDragPos = mouseLocation;
}

void PaintableClipboard::doNormalDragging(QPoint mouseLocation)
{
    m_dragX += (mouseLocation.x() - m_previousDragPos.x());
    m_dragY += (mouseLocation.y() - m_previousDragPos.y());

    m_previousDragPos = mouseLocation;

    update();
}

void PaintableClipboard::completeNormalDrag()
{
    m_previousDragPos = Constants::NullDragPoint;
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

bool PaintableClipboard::doNubblesDrag(QImage& canvasImage, QPointF mouseLocation, const float& zoom, const int& panOffsetX, const int& panOffsetY)
{
    const int offsetX = panOffsetX + m_dragX;
    const int offsetY = panOffsetY + m_dragY;
    QPoint center = QPoint(geometry().width() / 2, geometry().height() / 2);

    QPointF offsetMouseLocation = getPositionRelativeCenterdAndZoomedCanvas(mouseLocation, center, zoom, offsetX, offsetY);

    //If one of the nubbles is already dragging, continue dragging
    const QList nubbleKeys = m_dragNubbles.keys();
    for(const auto& nubblePos : nubbleKeys)
    {
        if(m_dragNubbles[nubblePos].isDragging())
        {
            m_dragNubbles[nubblePos].doDragging(offsetMouseLocation, m_dimensionsRect);
            doNubbleDragScale();
            return true;
        }
    }

    //Check if a nubble is being selected
    for(const auto& nubblePos : nubbleKeys)
    {
        if(m_dragNubbles[nubblePos].isStartDragging(offsetMouseLocation, getLocation(m_dimensionsRect, nubblePos), zoom))
        {
            if(!clipboardActive())
            {
                generateClipboard(canvasImage);
            }

            prepNubblesDrag();
            return true;
        }
    }

    return false;
}

void PaintableClipboard::reset()
{
    m_clipboardImage = QImage();
    m_backgroundImage = QImage();
    m_previousDragPos = Constants::NullDragPoint;
    m_dragX = 0;
    m_dragY = 0;
    m_pixels.clear();
    m_dimensionsRect = QRect();
    update();
}

QVector<QVector<bool>> listTo2dVector(const QVector<QPoint>& selectedPixels, const int& width, const int& height)
{
     QVector<QVector<bool>> selectedPixelsVector = QVector<QVector<bool>>(width, QVector<bool>(height, false));

     for(const QPoint& p : selectedPixels)
     {
        selectedPixelsVector[p.x()][p.y()] = true;
     }

     return selectedPixelsVector;
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

    //Outline
    m_bOutlineColorToggle = !m_bOutlineColorToggle;
    QPen selectionOutlinePen = QPen(m_bOutlineColorToggle ? Qt::black : Qt::white, 1/m_pParentCanvas->getZoom());
    painter.setPen(selectionOutlinePen);

    //2D vectorize selected pixels for quicker outline drawing
    QVector<QVector<bool>> selectedPixelsVector = listTo2dVector(m_pixels, m_clipboardImage.width(), m_clipboardImage.height());

    //Draw transparent selected pixels ~ todo - So inneficient! look for something else
    //Draw highlight outline
    for(QPoint& p : m_pixels)
    {
        if(m_clipboardImage != QImage())
        {
            if(m_clipboardImage.pixelColor(p.x(), p.y()).alpha() == 0)
            {
                painter.fillRect(QRect(p.x() + offsetX, p.y() + offsetY, 1, 1), m_backgroundImage.pixelColor(p.x(), p.y()));
            }
        }

        const int x = p.x();
        const int y = p.y();

        painter.fillRect(QRect(x + offsetX, y + offsetY, 1, 1), Constants::SelectionAreaColor);

        //border right
        if(x == selectedPixelsVector.size()-1 || (x + 1 < selectedPixelsVector.size() && !selectedPixelsVector[x+1][y]))
        {
            painter.drawLine(QPoint(x + offsetX + 1, y + offsetY), QPoint(x + offsetX + 1, y + offsetY + 1));
        }

        //border left
        if(x == 0 || (x > 0 && !selectedPixelsVector[x-1][y]))
        {
            painter.drawLine(QPoint(x + offsetX, y + offsetY), QPoint(x + offsetX, y + offsetY + 1));
        }

        //border bottom
        if(y == selectedPixelsVector[x].size()-1 || (y + 1 < selectedPixelsVector.size() && !selectedPixelsVector[x][y+1]))
        {
            painter.drawLine(QPoint(x + offsetX, y + offsetY + 1.0), QPoint(x + offsetX + 1.0, y + offsetY + 1.0));
        }

        //border top
        if(y == 0 || (y > 0 && !selectedPixelsVector[x][y-1]))
        {
            painter.drawLine(QPoint(x + offsetX, y + offsetY), QPoint(x + offsetX + 1.0, y + offsetY));
        }
    }

    //Draw nubbles to scale dimension of clipboard
    if(m_pixels.size() > 0)
    {
        QRectF translatedDimensions = m_dimensionsRect.translated((offsetX), (offsetY));

        const QList nubbleKeys = m_dragNubbles.keys();
        for(const auto& nubblePos : nubbleKeys)
        {
            m_dragNubbles[nubblePos].draw(painter, zoom, getLocation(translatedDimensions, nubblePos));
        }
    }
}

void PaintableClipboard::updateDimensionsRect()
{
    m_dimensionsRect = getPixelsDimensions(m_pixels);
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
