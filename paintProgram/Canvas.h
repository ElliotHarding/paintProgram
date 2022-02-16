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

class SelectedPixels : public QWidget
{
public:
    SelectedPixels(Canvas* parent, const uint width, const uint height);
    ~SelectedPixels();

    void clearAndResize(const uint width, const uint height);
    void clear();

    void operateOnSelectedPixels(std::function<void(int, int)> func);

    void addPixels(QRubberBand* newSelectionArea);
    void addPixels(std::vector<std::vector<bool>>& selectedPixels);
    void addPixels(QList<QPoint> pixels);

    bool isHighlighted(const uint x, const uint y);

private:
    std::vector<std::vector<bool>> m_selectedPixels;

    Canvas* m_pParentCanvas;

    bool m_bOutlineColorToggle = false;
    void paintEvent(QPaintEvent* paintEvent) override;
    QTimer* m_pOutlineDrawTimer;
};

/*
make dragging pixels an image + list so that we can set highlighted pixels even on transparent pixels
if make list of pixels in dragging can iterate through and set selected pixels on move.
instead of just selecting the non alpha 0 pixels
 */
class Clipboard
{
public:
    void generateClipboard(QImage& canvas, SelectedPixels* pSelectedPixels);

    QList<QPoint> m_pixels;
    QImage m_clipboardImage;
};

class PaintableClipboard : public Clipboard, public QWidget
{
public:
    PaintableClipboard(Canvas* parent);

    //Image
    void generateClipboard(QImage& canvas, SelectedPixels* pSelectedPixels);
    void setClipboard(Clipboard clipboard);
    void setImage(QImage image);
    Clipboard getClipboard();
    void dumpImage(QPainter& painter);
    bool isImageDefault();

    QList<QPoint> getPixels();
    QList<QPoint> getPixelsOffset();

    //Dragging
    bool isDragging();
    void startDragging(QPoint mouseLocation);
    void doDragging(QPoint mouseLocation);

    void reset();

private:

    void paintEvent(QPaintEvent* paintEvent) override;

    int m_dragX = 0;
    int m_dragY = 0;
    QPoint m_previousDragPos;

    Canvas* m_pParentCanvas;
};

class Canvas : public QTabWidget
{
    Q_OBJECT

public:
    Canvas(MainWindow* parent, QImage image);
    ~Canvas();

    void addedToTab();

    int width();
    int height();

    void updateText(QFont font);
    void writeText(QString letter, QFont font);

    QString getSavePath();
    void setSavePath(QString path);

    void updateSettings(int width, int height, QString name);

    void deleteKeyPressed();
    void copyKeysPressed();
    void cutKeysPressed();
    void pasteKeysPressed();
    void undoPressed();
    void redoPressed();

    QImage getImageCopy();

    float getZoom();
    void getPanOffset(float& offsetX, float& offsetY);

    void resizeEvent(QResizeEvent* event) override;

    void mouseMouseOnParentEvent(QMouseEvent* event);

public slots:
    void updateCurrentTool(Tool t);

signals:
    void selectionAreaResize(const int x, const int y);
    void mousePositionChange(const int x, const int y);
    void canvasSizeChange(const int x, const int y);

private:
    void paintEvent(QPaintEvent* paintEvent) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent *) override;

    //Mouse events and members
    void mousePressEvent(QMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool m_bMouseDown = false;

    //Drawing
    QImage m_canvasImage;
    QImage m_canvasBackgroundImage;
    QMutex m_canvasMutex;
    QString m_textToDraw = "";
    QPoint m_textDrawLocation;

    //Draw shape
    QPoint m_drawShapeOrigin = QPoint(0,0);

    //Undo/redo
    void recordImageHistory();//Function called when m_canvasMutex is locked
    std::vector<QImage> m_imageHistory;
    int m_imageHistoryIndex = 0;
    const uint m_c_maxHistory = 20;

    //Zooming
    float m_zoomFactor = 1;
    const float m_cZoomIncrement = 1.1;

    //Panning
    const QPoint m_c_nullPanPos = QPoint(-1,-1);
    QPoint m_previousPanPos = m_c_nullPanPos;
    float m_panOffsetX = 0;
    float m_panOffsetY = 0;

    //Selecting
    SelectedPixels* m_pSelectedPixels;
    QRubberBand* m_selectionTool = nullptr;
    QPoint m_selectionToolOrigin = QPoint(0,0);

    //Dragging/copy/paste
    PaintableClipboard* m_pClipboardPixels;

    //Geometry
    QPoint m_center;
    void updateCenter();

    Tool m_tool = TOOL_PAINT;

    MainWindow* m_pParent;

    QString m_savePath = "";
};

#endif // CANVAS_H
