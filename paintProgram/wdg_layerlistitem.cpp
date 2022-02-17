#include "wdg_layerlistitem.h"
#include "ui_wdg_layerlistitem.h"

WDG_LayerListItem::WDG_LayerListItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WDG_LayerListItem)
{
    ui->setupUi(this);
}

WDG_LayerListItem::~WDG_LayerListItem()
{
    delete ui;
}
