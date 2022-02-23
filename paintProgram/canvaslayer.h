#ifndef CANVASLAYER_H
#define CANVASLAYER_H

#include <QImage>

namespace Constants
{
const uint MaxImageHistory = 20;
}

struct CanvasLayerInfo
{
    QString m_name = "New Layer";
    bool m_enabled = true;
};

struct CanvasLayer
{
    CanvasLayerInfo m_info;
    QImage m_image;

    ///Undo/redo
    QList<QImage> m_history;
    uint m_historyIndex = 0;

    void recordHistory()
    {
        m_history.push_back(m_image);

        if(Constants::MaxImageHistory < m_history.size())
        {
            m_history.erase(m_history.begin());
        }

        m_historyIndex = m_history.size() - 1;
    }

    void redoHistory()
    {
        if(m_historyIndex < m_history.size() - 1)
        {
            m_image = m_history[size_t(++m_historyIndex)];
        }
    }

    void undoHistory()
    {
        if(m_historyIndex > 0)
        {
            m_image = m_history[size_t(--m_historyIndex)];
        }
    }
};

#endif // CANVASLAYER_H
