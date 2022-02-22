#include "dlg_layers.h"
#include "ui_dlg_layers.h"

#include "wdg_layerlistitem.h"

DLG_Layers::DLG_Layers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Layers)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    ui->listWidget_layers->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
}

DLG_Layers::~DLG_Layers()
{
    delete ui;
}

void DLG_Layers::on_btn_add_clicked()
{
    const uint layerPosition = ui->listWidget_layers->count();

    QListWidgetItem* item = new QListWidgetItem();
    WDG_LayerListItem* itemWidget = new WDG_LayerListItem(item, layerPosition);
    item->setSizeHint(QSize(itemWidget->geometry().width(), itemWidget->geometry().height()));
    ui->listWidget_layers->addItem(item);
    ui->listWidget_layers->setItemWidget(item, itemWidget);

    connect(itemWidget, SIGNAL(onDelete(QListWidgetItem*)), this, SLOT(onDelete(QListWidgetItem*)));
    connect(itemWidget, SIGNAL(onEnabledChaged(const uint, const bool)), this, SLOT(onEnabledChanged(const uint, const bool)));

    item = nullptr;
    itemWidget = nullptr;

    emit onLayerAdded(layerPosition);
}

void DLG_Layers::onDelete(QListWidgetItem* pListWidgetItem)
{
    const uint layerPosition = ui->listWidget_layers->row(pListWidgetItem);

    ui->listWidget_layers->removeItemWidget(pListWidgetItem);
    delete pListWidgetItem;

    emit onLayerDeleted(layerPosition);
}

void DLG_Layers::onEnabledChanged(QListWidgetItem* pListWidgetItem, const bool enabled)
{
    emit onLayerEnabledChanged(ui->listWidget_layers->row(pListWidgetItem), enabled);
}

void DLG_Layers::on_btn_merge_clicked()
{

}
