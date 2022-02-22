#ifndef CANVASLAYER_H
#define CANVASLAYER_H

#include <QImage>

struct CanvasLayerInfo
{
    QString m_name = "New Layer";
    bool m_enabled = true;
};

struct CanvasLayer : public CanvasLayerInfo
{
    QImage m_image;
};

#endif // CANVASLAYER_H
