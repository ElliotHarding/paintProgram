#include "canvas.h"
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>

Canvas::Canvas(MainWindow* parent, uint width, uint height) :
    m_pParent(parent),
    QTabWidget()
{
    // Initalize memory for pixel data
    for (uint x = 0; x < width; x++)
    {
        m_pixels.push_back(std::vector<Pixel>());
        for(uint y = 0; y < height; y++)
        {
            m_pixels[x].push_back(Pixel{0,0,0}); // 0,0,0 is black,  255,255,255 is white
        }
    }

    // Set half the canvas to the color red
    for (uint x = 0; x < width; x++)
    {
        for (uint y = 0; y < height/2; y++)
        {
            m_pixels[x][y].red = 255;
        }
    }

    setMouseTracking(true);
}

void Canvas::paintEvent(QPaintEvent *paintEvent)
{
    std::lock_guard<std::mutex> lock(m_pixelsMutex);

    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    painter.scale(m_zoomFactor, m_zoomFactor);

    qDebug() << paintEvent->region().boundingRect().x();
    qDebug() << paintEvent->region().boundingRect().y();
    qDebug() << paintEvent->region().boundingRect().width();

    for (size_t x = 0; x < m_pixels.size(); x++)
    {
        for(size_t y = 0; y < m_pixels[x].size(); y++)
        {
            QPen pen = QPen();
            //todo

            QRect rect = QRect(x, y, 1, 1);
            painter.fillRect(rect, QColor(m_pixels[x][y].red,m_pixels[x][y].green,m_pixels[x][y].blue));
        }
    }
}

void Canvas::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y() > 0)
    {
        m_zoomFactor += m_cZoomIncrement;
    }
    else if(event->angleDelta().y() < 0)
    {
         m_zoomFactor -= m_cZoomIncrement;
    }

    //Call to redraw
    update();
}

void Canvas::mousePressEvent(QMouseEvent *mouseEvent)
{
    m_bMouseDown = true;
    QPoint mouseLocation = getLocationFromMouseEvent(mouseEvent);
    updatePixel(mouseLocation.x(), mouseLocation.y());
}

void Canvas::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
    m_bMouseDown = false;
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bMouseDown)
    {
        QPoint mouseLocation = getLocationFromMouseEvent(event);
        updatePixel(mouseLocation.x(), mouseLocation.y());
    }
}

QPoint Canvas::getLocationFromMouseEvent(QMouseEvent *event)
{
    QTransform transform;
    transform.scale(m_zoomFactor, m_zoomFactor);
    return transform.inverted().map(QPoint(event->x(), event->y()));
}

void Canvas::updatePixel(uint posX, uint posY)
{
    std::lock_guard<std::mutex> lock(m_pixelsMutex);

    //Check positions are in bounds of vector
    if(posX <= m_pixels.size() && posY <= m_pixels[0].size())
    {
        //Get color from color picker dialog in MainWindow
        QColor col = m_pParent->getSelectedColor();

        Pixel newPixel;
        newPixel.blue = col.blue();
        newPixel.red = col.red();
        newPixel.green = col.green();

        m_pixels[posX][posY] = newPixel;

        //Call to redraw
        update(posX, posY, 1, 1);
    }
}
