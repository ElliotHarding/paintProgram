#include "wdg_layerlistitem.h"
#include "ui_wdg_layerlistitem.h"

WDG_LayerListItem::WDG_LayerListItem(QListWidgetItem* pListWidgetItem) :
    m_pListWidgetItem(pListWidgetItem),
    ui(new Ui::WDG_LayerListItem)
{
    ui->setupUi(this);
}

WDG_LayerListItem::~WDG_LayerListItem()
{
    delete ui;
}

void WDG_LayerListItem::on_btn_close_clicked()
{
    emit onDelete(m_pListWidgetItem);
}
