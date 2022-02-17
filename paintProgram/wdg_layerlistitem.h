#ifndef WDG_LAYERLISTITEM_H
#define WDG_LAYERLISTITEM_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
class WDG_LayerListItem;
}

class WDG_LayerListItem : public QWidget
{
    Q_OBJECT

public:
    explicit WDG_LayerListItem(QListWidgetItem* pListWidgetItem);
    ~WDG_LayerListItem();

signals:
    void onDelete(QListWidgetItem* pListWidgetItem);

private slots:
    void on_btn_close_clicked();

private:
    Ui::WDG_LayerListItem *ui;

    QListWidgetItem* m_pListWidgetItem;
};

#endif // WDG_LAYERLISTITEM_H
