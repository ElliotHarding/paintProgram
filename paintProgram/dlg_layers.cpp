#include "dlg_layers.h"
#include "ui_dlg_layers.h"

#include "wdg_layerlistitem.h"

DLG_Layers::DLG_Layers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Layers)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_Layers::~DLG_Layers()
{
    delete ui;
}

void DLG_Layers::on_btn_add_clicked()
{
    QListWidgetItem* item = new QListWidgetItem();
    WDG_LayerListItem* itemWidget = new WDG_LayerListItem(item);
    item->setSizeHint(QSize(itemWidget->geometry().width(), itemWidget->geometry().height()));
    ui->listWidget_layers->addItem(item);
    ui->listWidget_layers->setItemWidget(item, itemWidget);

    connect(itemWidget, SIGNAL(onDelete(QListWidgetItem*)), this, SLOT(onDelete(QListWidgetItem*)));

    item = nullptr;
    itemWidget = nullptr;
}

void DLG_Layers::onDelete(QListWidgetItem* pListWidgetItem)
{
    ui->listWidget_layers->removeItemWidget(pListWidgetItem);
    delete pListWidgetItem;
}

void DLG_Layers::on_btn_merge_clicked()
{

}
