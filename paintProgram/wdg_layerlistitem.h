#ifndef WDG_LAYERLISTITEM_H
#define WDG_LAYERLISTITEM_H

#include <QWidget>

namespace Ui {
class WDG_LayerListItem;
}

class WDG_LayerListItem : public QWidget
{
    Q_OBJECT

public:
    explicit WDG_LayerListItem(QWidget *parent = nullptr);
    ~WDG_LayerListItem();

private:
    Ui::WDG_LayerListItem *ui;
};

#endif // WDG_LAYERLISTITEM_H
