#include "wdg_layerlistitem.h"
#include "ui_wdg_layerlistitem.h"

WDG_LayerListItem::WDG_LayerListItem(QListWidgetItem* pListWidgetItem, const uint id) :
    m_pListWidgetItem(pListWidgetItem),
    m_id(id),
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
    emit onDelete(m_pListWidgetItem, m_id);
}

void WDG_LayerListItem::on_checkBox_enabled_stateChanged(int enabled)
{
    emit onEnabledChaged(m_id, enabled);
}
