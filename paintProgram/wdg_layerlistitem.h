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

    void setSelected(bool isSelected);

signals:
    void onDelete(QListWidgetItem* pListWidgetItem);
    void onEnabledChaged(QListWidgetItem* pListWidgetItem, const bool enabled);
    void onTextChanged(QListWidgetItem* pListWidgetItem, QString text);

private slots:
    void on_btn_close_clicked();
    void on_checkBox_enabled_stateChanged(int enabled);

    void on_textEdit_name_textChanged();

private:
    Ui::WDG_LayerListItem *ui;

    QListWidgetItem* m_pListWidgetItem;
};

#endif // WDG_LAYERLISTITEM_H
