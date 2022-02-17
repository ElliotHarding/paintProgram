#ifndef CANVAS_H
#define CANVAS_H

#include <QTabWidget>
#include <QRubberBand>
#include <QMutex>
#include <vector>
#include <QPainter>
#include <QTimer>
#include <functional>

#include "tools.h"

class Canvas;
class MainWindow;

/*
add list of pixels aswell to selectedpixels,
so can get number of pixels quick (so can check if no pixels are selected, so do action on all imnage)
so can draw pixels quicker
 */

///Holds selected pixels of parent canvas (also paints these pixels)
class SelectedPixels : public QWidget
{
public:
    SelectedPixels(Canvas* parent, const uint width, const uint height);
    ~SelectedPixels();

    ///Clearing
    void clearAndResize(const uint width, const uint height);
    void clear();

    ///Pixel operations
    void operateOnSelectedPixels(std::function<void(int, int)> func);

    ///Adding pixels
    void addPixels(QRubberBand* newSelectionArea);
    void addPixels(std::vector<std::vector<bool>>& selectedPixels);
    void addPixels(QList<QPoint> pixels);

    ///Pixel info
    bool isHighlighted(const uint x, const uint y);
    bool containsPixels();

private:
    ///Containers of selected pixels (two differnt for speed instead of memory)
    /// (It is slower adding pixels to two containers but speed saved elsewhere outweighs it)
    std::vector<std::vector<bool>> m_selectedPixels;//2d array for quick access to specific pixels
    QList<QPoint> m_selectedPixelsList;//List for quick iteration of only selected pixels

    Canvas* m_pParentCanvas;

    ///Painting
    bool m_bOutlineColorToggle = false;
    void paintEvent(QPaintEvent* paintEvent) override;
    QTimer* m_pOutlineDrawTimer;//Calls draw of outline of selected pixels every interval
};

///Clipboard (Image + Pixel info) used for copying/cutting/pasting
class Clipboard
{
public:
    void generateClipboard(QImage& canvas, SelectedPixels* pSelectedPixels);

    QList<QPoint> m_pixels;
    QImage m_clipboardImage;
};

///A clipboard that also paints. Used for dragging around or cutting/pasting
class PaintableClipboard : public Clipboard, public QWidget
{
public:
    PaintableClipboard(Canvas* parent);

    ///Clipboard (image + pixels)
    void generateClipboard(QImage& canvas, SelectedPixels* pSelectedPixels);
    void setClipboard(Clipboard clipboard);
    Clipboard getClipboard();

    ///Image
    void setImage(QImage image);    
    bool dumpImage(QPainter& painter);//Returns false if no image dumped
    bool isImageDefault();

    ///Pixels
    QList<QPoint> getPixels();
    QList<QPoint> getPixelsOffset();

    ///Dragging
    bool isDragging();
    void startDragging(QPoint mouseLocation);
    void doDragging(QPoint mouseLocation);

    ///Reset/clear
    void reset();

private:
    void paintEvent(QPaintEvent* paintEvent) override;

    int m_dragX = 0;
    int m_dragY = 0;
    QPoint m_previousDragPos;

    Canvas* m_pParentCanvas;
};


///Tab widget in charge of displaying & interacting with the image edited by paint program
class Canvas : public QTabWidget
{
    Q_OBJECT

public:
    Canvas(MainWindow* parent, QImage image);
    ~Canvas();

    ///Image stuff & Saving/loading
    int width();
    int height();
    QImage getImageCopy();
    QString getSavePath();
    void setSavePath(QString path);   

    ///Events
    void onAddedToTab();
    void onUpdateSettings(int width, int height, QString name);
    void onCurrentToolUpdated(const Tool t);
    void onParentMouseMove(QMouseEvent* event);

    ///Text writing events
    void onUpdateText(QFont font);
    void onWriteText(QString letter, QFont font);

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
    void onRedLimit(const int value);
    void onBlueLimit(const int value);
    void onGreenLimit(const int value);
    void onInkSketch();
    void onConfirmEffects();
    void onCancelEffects();

    ///Qt events
    void resizeEvent(QResizeEvent* event) override;

    ///Stuff called by childen
    float getZoom();
    void getPanOffset(float& offsetX, float& offsetY);

signals:
    void selectionAreaResize(const int x, const int y);
    void mousePositionChange(const int x, const int y);
    void canvasSizeChange(const int x, const int y);

private:
    void paintEvent(QPaintEvent* paintEvent) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent *) override;

    ///Mouse events and members
    void mousePressEvent(QMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool m_bMouseDown = false;

    ///Drawing
    QImage m_canvasImage;
    QImage m_canvasBackgroundImage;    
    QMutex m_canvasMutex;
    QString m_textToDraw = "";
    QPoint m_textDrawLocation;
    QImage m_beforeEffectsImage;
    QImage getCanvasImageBeforeEffects();//Requires m_canvasMutex to be locked!

    ///Draw shape
    QPoint m_drawShapeOrigin = QPoint(0,0);

    ///Undo/redo
    void recordImageHistory();//Function called when m_canvasMutex is locked
    std::vector<QImage> m_imageHistory;
    int m_imageHistoryIndex = 0;
    const uint m_c_maxHistory = 20;

    ///Zooming
    float m_zoomFactor = 1;
    const float m_cZoomIncrement = 1.1;

    ///Panning
    const QPoint m_c_nullPanPos = QPoint(-1,-1);
    QPoint m_previousPanPos = m_c_nullPanPos;
    float m_panOffsetX = 0;
    float m_panOffsetY = 0;

    ///Selecting
    SelectedPixels* m_pSelectedPixels;
    QRubberBand* m_selectionTool = nullptr;
    QPoint m_selectionToolOrigin = QPoint(0,0);

    ///Dragging/copy/paste
    PaintableClipboard* m_pClipboardPixels;

    ///Geometry
    QPoint m_center;
    void updateCenter();

    Tool m_tool = TOOL_PAINT;

    MainWindow* m_pParent;

    QString m_savePath = "";
};

#endif // CANVAS_H
