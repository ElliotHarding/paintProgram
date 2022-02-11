#ifndef CANVAS_H
#define CANVAS_H

#include <QTabWidget>
#include <QRubberBand>
#include <QMutex>
#include <vector>
#include <QPainter>

#include "mainwindow.h"

class Canvas: public QTabWidget
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
    QMutex m_canvasMutex;
    QString m_textToDraw = "";
    QPoint m_textDrawLocation;

    //Draw shape
    QPoint m_drawShapeOrigin = QPoint(0,0);

    //Undo/redo
    void recordImageHistory();//Function called when m_canvasMutex is locked
    std::vector<QImage> m_imageHistory;
    int m_imageHistoryIndex = 0;
    const int m_c_maxHistory = 20;

    //Zooming
    float m_zoomFactor = 1;
    const float m_cZoomIncrement = 1.1;

    //Panning
    const QPoint m_c_nullPanPos = QPoint(-1,-1);
    QPoint m_previousPanPos = m_c_nullPanPos;
    float m_panOffsetX = 0;
    float m_panOffsetY = 0;

    //Selecting
    std::vector<std::vector<bool>> m_selectedPixels;
    QRubberBand* m_selectionTool = nullptr;
    QPoint m_selectionToolOrigin = QPoint(0,0);

    //Dragging/copy/paste
    QPoint m_previousDragPos;
    QImage m_clipboardImage;
    int m_dragOffsetX = 0;
    int m_dragOffsetY = 0;

    //Geometry
    QPoint m_center;
    void updateCenter();

    Tool m_tool = TOOL_PAINT;

    MainWindow* m_pParent;

    QString m_savePath = "";
};

class SelectedPixels
{
private:
    std::vector<std::vector<bool>> m_selectedPixels;
    QImage m_image;

public:
    SelectedPixels(int width, int height);

    void clearAndResize(int width, int height);
    void clear();

    void addPixels(QRubberBand* newSelectionArea);
    void addNonAlpha0Pixels(QImage& image);
    void addNonAlpha0PixelsWithOffset(QImage& image, int offsetX, int offsetY);

    void redraw();
    QImage& getImage();

    void fillColor(QPainter painter, QColor color);

    bool isHighlighted(int x, int y);
    std::vector<std::vector<bool>>& getPixels();

};

#endif // CANVAS_H
