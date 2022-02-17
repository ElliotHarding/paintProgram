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
    const uint id = m_idNumber++;
    WDG_LayerListItem* itemWidget = new WDG_LayerListItem(item, id);
    item->setSizeHint(QSize(itemWidget->geometry().width(), itemWidget->geometry().height()));
    ui->listWidget_layers->addItem(item);
    ui->listWidget_layers->setItemWidget(item, itemWidget);

    connect(itemWidget, SIGNAL(onDelete(QListWidgetItem*, const uint)), this, SLOT(onDelete(QListWidgetItem*, const uint)));
    connect(itemWidget, SIGNAL(onEnabledChaged(const uint, const bool)), this, SLOT(onEnabledChanged(const uint, const bool)));

    item = nullptr;
    itemWidget = nullptr;

    emit onLayerAdded(id);
}

void DLG_Layers::onDelete(QListWidgetItem* pListWidgetItem, const uint id)
{
    ui->listWidget_layers->removeItemWidget(pListWidgetItem);
    delete pListWidgetItem;

    emit onLayerDeleted(id);
}

void DLG_Layers::onEnabledChanged(const uint id, const bool enabled)
{
    emit onLayerEnabledChanged(id, enabled);
}

void DLG_Layers::on_btn_merge_clicked()
{

}
