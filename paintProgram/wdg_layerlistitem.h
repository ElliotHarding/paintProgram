#ifndef WDG_LAYERLISTITEM_H
#define WDG_LAYERLISTITEM_H

#include <QWidget>
#include <QListWidgetItem>

#include "canvaslayer.h"

namespace Ui {
class WDG_LayerListItem;
}

class WDG_LayerListItem : public QWidget
{
    Q_OBJECT

public:
    WDG_LayerListItem(QListWidgetItem* pListWidgetItem, CanvasLayerInfo info);

    ~WDG_LayerListItem();

signals:
    void onDelete(QListWidgetItem* pListWidgetItem);
    void onEnabledChaged(QListWidgetItem* pListWidgetItem, const bool enabled);

private slots:
    void on_btn_close_clicked();
    void on_checkBox_enabled_stateChanged(int enabled);

private:
    Ui::WDG_LayerListItem *ui;

    QListWidgetItem* m_pListWidgetItem;
};

#endif // WDG_LAYERLISTITEM_H
