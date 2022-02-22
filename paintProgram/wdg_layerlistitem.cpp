#include "wdg_layerlistitem.h"
#include "ui_wdg_layerlistitem.h"

WDG_LayerListItem::WDG_LayerListItem(QListWidgetItem *pListWidgetItem, CanvasLayerInfo info) :
    m_pListWidgetItem(pListWidgetItem),
    ui(new Ui::WDG_LayerListItem)
{
    ui->setupUi(this);
    ui->checkBox_enabled->setCheckState(info.m_enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->textEdit_name->setText(info.m_name);
}

WDG_LayerListItem::~WDG_LayerListItem()
{
    delete ui;
}

void WDG_LayerListItem::on_btn_close_clicked()
{
    emit onDelete(m_pListWidgetItem);
}

void WDG_LayerListItem::on_checkBox_enabled_stateChanged(int enabled)
{
    emit onEnabledChaged(m_pListWidgetItem, enabled);
}

void WDG_LayerListItem::on_textEdit_name_textChanged()
{
    emit onTextChanged(m_pListWidgetItem, ui->textEdit_name->toPlainText());
}
