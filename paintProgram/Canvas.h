#ifndef CANVAS_H
#define CANVAS_H

#include <QTabWidget>
#include <mutex>

#include <vector>

#include "mainwindow.h"
#include "Pixel.h"

class Canvas: public QTabWidget
{
    Q_OBJECT
public:
    Canvas(MainWindow* parent, uint width, uint height);

private:
    void paintEvent(QPaintEvent* paintEvent) override;
    void wheelEvent(QWheelEvent* event) override;

    //Mouse events and members
    void mousePressEvent(QMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    QPoint getLocationFromMouseEvent(QMouseEvent* event);
    bool m_bMouseDown = false;

    void paintEvent(uint posX, uint posY);

    std::vector<std::vector<Pixel>> m_pixels;
    std::mutex m_pixelsMutex;

    float m_zoomFactor = 1;
    const float m_cZoomIncrement = 0.1;

    MainWindow* m_pParent;

};

#endif // CANVAS_H
