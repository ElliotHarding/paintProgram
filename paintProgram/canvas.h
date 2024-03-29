#ifndef CANVAS_H
#define CANVAS_H

#include <QTabWidget>
#include <QRubberBand>
#include <vector>
#include <QPainter>
#include <QTimer>
#include <functional>
#include <QMap>

#include "tools.h"
#include "canvaslayer.h"

class Canvas;
class MainWindow;
class PaintableClipboard;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Clipboard
///
///Clipboard (Image + Pixel info) used for copying/cutting/pasting
class Clipboard
{
public:
    QVector<QPoint> m_pixels;
    QImage m_clipboardImage = QImage();
    int m_dragX = 0;
    int m_dragY = 0;
};

enum DragNubblePos
{
    TopLeft,
    TopMiddle,
    TopRight,
    LeftMiddle,
    RightMiddle,
    BottomLeft,
    BottomMiddle,
    BottomRight
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ResizeNubble
///
class ResizeNubble
{
public:
    ResizeNubble(std::function<void(QRect&, const QPointF&)> operation);
    ResizeNubble(){}

    ///Dragging
    void setDragging(const bool dragging);
    bool isStartDragging(const QPointF &mouseLocation, QPointF location, const float& zoom);
    bool isDragging();
    void doDragging(const QPointF &mouseLocation, QRect& rect);

    void draw(QPainter &painter, const float& zoom, const QPointF location);

private:
    std::function<void (QRect&, const QPointF&)> m_operation;

    bool m_bIsDragging = false;

    inline static QImage m_image = QImage();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PaintableClipboard
///
///A clipboard that also paints. Used for dragging, selecting cutting, pasting
class PaintableClipboard : public QWidget, public Clipboard
{
    Q_OBJECT

public:
    PaintableClipboard(Canvas* parent, const int& canvasWidth, const int& canvasHeight);
    ~PaintableClipboard();

    ///Clipboard (image + pixels)
    void generateClipboardSteal(QImage& canvas);
    void setClipboard(Clipboard clipboard);
    Clipboard getClipboard();
    bool clipboardActive();

    ///Image
    void setImage(QImage& image);
    QImage& getImage();
    bool dumpImage(QPainter& painter);//Returns false if no image dumped

    ///Pixel info
    bool containsPixels();
    QVector<QPoint> getPixels();

    ///Pixel operations
    void operateOnSelectedPixels(std::function<void(int, int)> func);

    ///Adding pixels
    void addPixels(QImage& canvas, QRubberBand* newSelectionArea);
    void addPixels(QImage& canvas, QVector<QVector<bool>>& selectedPixels);

    ///Dragging
    void checkDragging(QImage& canvasImage, QPoint mouseLocation, QPointF globalMouseLocation);
    void checkRotating(QImage& canvasImage, QPoint mouseLocation);
    bool checkFinishOperation();

    ///Reset/clear
    void reset();

    ///Parent stuff
    void updateParentCanvasSize(const int& width, const int& height);
    void updateParentZoom(const float& zoom);
    void updateParentPanOffset(const int& panOffsetX, const int& panOffsetY);

private slots:
    void onSwitchOutlineColor();

private:
    ///Drawing
    void paintEvent(QPaintEvent* paintEvent) override;
    bool m_bOutlineColorToggle = false;
    QTimer* m_pOutlineDrawTimer;//Calls draw of outline of selected pixels every interval
    QImage m_backgroundImage;

    ///Pixels
    void addImageToActiveClipboard(QImage& newPixelsImage);
    bool isHighlighted(const int& x, const int& y);
    QVector<QPoint> getPixelsOffset();

    ///Pixels borders
    QList<QPair<QPoint, QPoint>> m_pixelBorders;
    void updatePixelBorders();

    enum OperationMode
    {
        NoOperation,
        DragOperation,
        ResizeOperation,
        RotateOperation
    };
    OperationMode m_operationMode = NoOperation;

    ///Normal Dragging
    QPoint m_previousDragPos;
    bool checkStartNormalDragging(QImage& canvas, QPoint mouseLocation);
    void doNormalDragging(QPoint mouseLocation);

    ///Resize & rotate dragging (shared stuff)
    void prepResizeOrRotateDrag();
    QImage m_clipboardImageBeforeOperation = QImage();
    QImage m_clipboardImageBeforeOperationTransparent = QImage();

    ///Resize nubble dragging
    QMap<DragNubblePos, ResizeNubble> m_resizeNubbles;
    QRect m_dimensionsRectBeforeOperation = QRect();
    void doResizeDrag(QPointF mouseLocation);
    bool checkStartResizeDrag(QImage& canvasImage, QPointF mouseLocation);
    void doResizeDragScale();

    ///Rotate dragging
    bool checkStartRotateDrag(QImage& canvasImage, QPointF mouseLocation);
    void doRotateDrag(QPointF mouseLocation);

    ///Nubble dimensions rect
    QRect m_dimensionsRect = QRect();
    void updateDimensionsRect();

    ///Parent stuff
    Canvas* m_pParentCanvas;
    int m_parentCanvasWidth = 0;
    int m_parentCanvasHeight = 0;
    int m_parentPanOffsetX = 0;
    int m_parentPanOffsetY = 0;
    float m_parentZoom = 0;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CanvasHistoryItem
///
class CanvasHistoryItem
{
public:
    QList<CanvasLayer> m_layers;
    Clipboard m_clipboard;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CanvasHistory
///
///Holds the history of actions on the canvas ~ may store only changes between
///  saves later (instead of entire canvas and clipboard each time)
class CanvasHistory
{
public:
    void recordHistory(CanvasHistoryItem canvasSnapShot);
    bool redoHistory(CanvasHistoryItem& canvasSnapShot);
    bool undoHistory(CanvasHistoryItem& canvasSnapShot);

private:
    QList<CanvasHistoryItem> m_history;
    uint m_historyIndex = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Canvas
///
///Tab widget in charge of displaying & interacting with the image edited by paint program
class Canvas : public QTabWidget
{
    Q_OBJECT

public:
    Canvas(MainWindow* parent, const int width, const int height);
    Canvas(MainWindow* parent, QString& filePath, bool& loadSuccess);
    ~Canvas();

    ///Image stuff & Saving/loading
    int width();
    int height();
    QImage getImageCopy();
    QString getSavePath();
    bool save(QString path);

    ///Layer stuff
    void onLayerAdded();
    void onLayerDeleted(const uint index);
    void onLayerEnabledChanged(const uint index, const bool enabled);
    void onLayerTextChanged(const uint index, QString text);
    void onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB);
    void onLayerMoveUp(const uint index);
    void onLayerMoveDown(const uint index);
    void onSelectedLayerChanged(const uint index);
    void onLoadLayer(CanvasLayer canvasLayer);

    ///Events
    void onAddedToTab();
    void onUpdateSettings(int width, int height, QString name);
    void onCurrentToolUpdated(const Tool t);
    void onParentMouseMove(QMouseEvent* event);
    void onParentMouseScroll(QWheelEvent* event);

    ///Text writing events
    void onUpdateText();
    void onWriteText(QString letter);

    ///Shortcut/keyboard events
    void onDeleteKeyPressed();
    void onCopyKeysPressed();
    void onCutKeysPressed();
    void onPasteKeysPressed();
    void onUndoPressed();
    void onRedoPressed();

    ///Effects events
    void onBlackAndWhite();
    void onInvert();
    void onBrightness(const int value);
    void onContrast(const int value);
    void onOutlineEffect(const int value);
    void onSketchEffect(const int value);
    void onNormalBlur(const int& difference, const int& averageArea, const bool& includeTransparent);
    void onColorMultipliers(const int redXred, const int redXgreen, const int redXblue,
                            const int greenXred, const int greenXgreen, const int greenXblue,
                            const int blueXred, const int blueXgreen, const int blueXblue,
                            const int xTransparent);
    void onHueSaturation(const int& hue, const int& saturation);
    void onConfirmEffects();
    void onCancelEffects();

    ///Qt events
    void resizeEvent(QResizeEvent* event) override;

    ///Stuff called by childen
    Tool currentTool();

signals:
    void selectionAreaResize(const int x, const int y);
    void mousePositionChange(const int x, const int y);
    void canvasSizeChange(const int x, const int y);

private:
    void init(uint width, uint height);

    void paintEvent(QPaintEvent* paintEvent) override;    
    void showEvent(QShowEvent *) override;

    ///Mouse events and members
    void mousePressEvent(QMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool m_bMouseDown = false;
    bool m_bMiddleMouseDown = false;

    ///Drawing
    QList<CanvasLayer> m_canvasLayers;
    uint m_selectedLayer;
    QImage m_canvasBackgroundImage;    
    QImage m_beforeEffectsImage;
    QImage getCanvasImageBeforeEffects();
    Clipboard m_beforeEffectsClipboard;
    Clipboard getClipboardBeforeEffects();

    ///Drawing text
    QString m_textToDraw = "";
    QPoint m_textDrawLocation;

    ///Draw shape
    QPoint m_drawShapeOrigin = QPoint(0,0);

    ///Zooming
    float m_zoomFactor = 1;

    ///Panning
    const QPoint m_c_nullPanPos = QPoint(-1,-1);
    QPoint m_previousPanPos = m_c_nullPanPos;
    int m_panOffsetX = 0;
    int m_panOffsetY = 0;

    ///Selecting
    QRubberBand* m_selectionTool = nullptr;
    QPoint m_selectionToolOrigin = QPoint(0,0);

    ///Dragging/copy/paste
    PaintableClipboard* m_pClipboardPixels;

    ///Undo/redo
    CanvasHistory m_canvasHistory;
    CanvasHistoryItem getSnapshot();

    ///Geometry
    uint m_canvasWidth;
    uint m_canvasHeight;
    QPoint m_center;//Center of widget - not canvas
    void updateCenter();

    Tool m_tool = TOOL_PAINT;

    MainWindow* m_pParent;

    QString m_savePath = "";
};

#endif // CANVAS_H
