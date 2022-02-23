#include "dlg_layers.h"
#include "ui_dlg_layers.h"

#include "wdg_layerlistitem.h"

DLG_Layers::DLG_Layers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Layers)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    ui->listWidget_layers->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);

    connect(ui->listWidget_layers, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChanged(int)));
}

DLG_Layers::~DLG_Layers()
{
    delete ui;
}

void DLG_Layers::setLayers(QList<CanvasLayerInfo> layerInfo, uint selectedLayer)
{
    //Dont want the clearing of the layers to signal the canvas of a selected row change since canvas is calling setLayers with its mutex locked.
    disconnect(ui->listWidget_layers, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChanged(int)));

    //Clear current displayed layers and add new ones
    ui->listWidget_layers->clear();
    for(CanvasLayerInfo& layer : layerInfo)
    {
        addLayer(layer);
    }

    //set current selected layer and update layers to highlight selected one
    ui->listWidget_layers->setCurrentRow(selectedLayer);
    drawLayerSelections(selectedLayer);

    //Reconnet current row changed signal and slot
    connect(ui->listWidget_layers, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChanged(int)));
}

void DLG_Layers::on_btn_add_clicked()
{
    CanvasLayerInfo info;
    addLayer(info);

    emit onLayerAdded();
}

void DLG_Layers::onDelete(QListWidgetItem* pListWidgetItem)
{
    //Dont ever remove the last layer
    if(ui->listWidget_layers->count() > 1)
    {
        const uint layerIndex = ui->listWidget_layers->row(pListWidgetItem);

        ui->listWidget_layers->removeItemWidget(pListWidgetItem);
        delete pListWidgetItem;

        emit onLayerDeleted(layerIndex);

        if(layerIndex == 0)
        {
            currentRowChanged(0);
        }
    }
}

void DLG_Layers::onEnabledChanged(QListWidgetItem* pListWidgetItem, const bool enabled)
{
    emit onLayerEnabledChanged(ui->listWidget_layers->row(pListWidgetItem), enabled);
}

void DLG_Layers::onTextChanged(QListWidgetItem *pListWidgetItem, QString text)
{
    emit onLayerTextChanged(ui->listWidget_layers->row(pListWidgetItem), text);
}

void DLG_Layers::currentRowChanged(int currentRow)
{
    drawLayerSelections(currentRow);

    emit onSelectedLayerChanged(currentRow);
}

void DLG_Layers::addLayer(CanvasLayerInfo info)
{
    QListWidgetItem* item = new QListWidgetItem();
    WDG_LayerListItem* itemWidget = new WDG_LayerListItem(item, info);
    item->setSizeHint(QSize(itemWidget->geometry().width(), itemWidget->geometry().height()));
    ui->listWidget_layers->addItem(item);
    ui->listWidget_layers->setItemWidget(item, itemWidget);

    connect(itemWidget, SIGNAL(onDelete(QListWidgetItem*)), this, SLOT(onDelete(QListWidgetItem*)));
    connect(itemWidget, SIGNAL(onEnabledChaged(QListWidgetItem*, const bool)), this, SLOT(onEnabledChanged(QListWidgetItem*, const bool)));
    connect(itemWidget, SIGNAL(onTextChanged(QListWidgetItem*, QString)), this, SLOT(onTextChanged(QListWidgetItem*, QString)));

    item = nullptr;
    itemWidget = nullptr;
}

void DLG_Layers::drawLayerSelections(uint currentRow)
{
    //Loop through all layer widgets set background to white
    for(int row = 0; row < ui->listWidget_layers->count(); row++)
    {
        QListWidgetItem *item = ui->listWidget_layers->item(row);
        WDG_LayerListItem* itemWidget = dynamic_cast<WDG_LayerListItem*>(ui->listWidget_layers->itemWidget(item));
        if(itemWidget)
        {
            itemWidget->setSelected(row == currentRow);
        }
    }
}

void DLG_Layers::on_btn_merge_clicked()
{
    const int currentRow = ui->listWidget_layers->currentRow();

    //If we have a layer below the current one
    if(currentRow > -1 && currentRow < ui->listWidget_layers->count() - 1)
    {
        emit onLayerMergeRequested(currentRow, currentRow+1);
    }
}

void DLG_Layers::on_btn_moveUp_clicked()
{

}

void DLG_Layers::on_btn_moveDown_clicked()
{

}
