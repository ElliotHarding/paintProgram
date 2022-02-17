#include "dlg_layers.h"
#include "ui_dlg_layers.h"

#include <QListWidgetItem>

#include "wdg_layerlistitem.h"

DLG_Layers::DLG_Layers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Layers)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    QListWidgetItem* item1 = new QListWidgetItem();
    WDG_LayerListItem* item1Widget = new WDG_LayerListItem();
    item1->setSizeHint(QSize(item1Widget->geometry().width(), item1Widget->geometry().height()));
    ui->listWidget_layers->addItem(item1);
    ui->listWidget_layers->setItemWidget(item1, item1Widget);

    QListWidgetItem* item2 = new QListWidgetItem();
    WDG_LayerListItem* item2Widget = new WDG_LayerListItem();
    item2->setSizeHint(QSize(item1Widget->geometry().width(), item1Widget->geometry().height()));
    ui->listWidget_layers->addItem(item2);
    ui->listWidget_layers->setItemWidget(item2, item2Widget);
}

DLG_Layers::~DLG_Layers()
{
    delete ui;
}

void DLG_Layers::on_btn_merge_clicked()
{

}
