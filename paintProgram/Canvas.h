#ifndef CANVAS_H
#define CANVAS_H

#include <QTabWidget>
#include <QRubberBand>
#include <mutex>
#include <vector>
#include <QPainter>

#include "mainwindow.h"

class Canvas: public QTabWidget
{
    Q_OBJECT

public:
    Canvas(MainWindow* parent, QImage image);
    ~Canvas();

    void deleteKeyPressed();
    void copyKeysPressed();
    void cutKeysPressed();
    void pasteKeysPressed();
    void undoPressed();
    void redoPressed();

    QImage getImageCopy();

public slots:
    void updateCurrentTool(Tool t);

private:
    void paintEvent(QPaintEvent* paintEvent) override;
    void wheelEvent(QWheelEvent* event) override;

    //Mouse events and members
    void mousePressEvent(QMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    QPoint getLocationFromMouseEvent(QMouseEvent* event);
    bool m_bMouseDown = false;

    //Drawing
    void paintBrush(uint posX, uint posY, QColor col);
    void drawTransparentPixels(QPainter& painter, float offsetX, float offsetY);
    QImage m_canvasImage;
    std::mutex m_canvasMutex;
    const QColor m_c_transparentGrey = QColor(190,190,190,255);
    const QColor m_c_transparentWhite = QColor(255,255,255,255);

    //Undo/redo
    void recordImageHistory();
    std::vector<QImage> m_imageHistory;
    int m_imageHistoryIndex = 0;
    const int m_c_maxHistory = 20;

    //Zooming
    float m_zoomFactor = 1;
    const float m_cZoomIncrement = 0.1;

    //Panning
    const QPoint m_c_nullPanPos = QPoint(-1,-1);
    QPoint m_previousPanPos = m_c_nullPanPos;
    float m_panOffsetX = 0;
    float m_panOffsetY = 0;
    std::mutex m_panOffsetMutex;

    //Selecting
    void spreadSelectArea(int x, int y);
    void releaseSelect();
    QList<QPoint> m_selectedPixels;
    QRubberBand* m_selectionTool = nullptr;
    QPoint m_selectionToolOrigin = QPoint(0,0);
    const QColor m_c_selectionBorderColor = Qt::blue; //todo ~ not const because future cahnges planned (if highlight color and background color are the same)
    const QColor m_c_selectionAreaColor = QColor(0,40,100, 50);

    //Dragging/copy/paste
    QImage genClipBoard();
    void dragPixels(QPoint mousePosition);
    const QPoint m_c_nullDragPos = QPoint(0,0);
    QPoint m_previousDragPos = m_c_nullDragPos;
    QImage m_clipboardImage;
    int m_dragOffsetX = 0;
    int m_dragOffsetY = 0;
    std::mutex m_dragOffsetMutex;

    //Bucket
    void floodFillOnSimilar(QImage& image, QColor newColor, int startX, int startY, int sensitivity);

    Tool m_tool = TOOL_PAINT;

    MainWindow* m_pParent;
};

#endif // CANVAS_H
