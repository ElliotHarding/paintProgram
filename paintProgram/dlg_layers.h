#ifndef DLG_LAYERS_H
#define DLG_LAYERS_H

#include <QDialog>
#include <QListWidgetItem>

#include "canvaslayer.h"

namespace Ui {
class DLG_Layers;
}

class DLG_Layers : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Layers(QWidget *parent = nullptr);
    ~DLG_Layers();

    void setLayers(QList<CanvasLayerInfo> layerInfo, uint selectedLayer);

signals:
    void onLayerAdded();
    void onLayerDeleted(const uint index);
    void onLayerEnabledChanged(const uint index, const bool enabled);
    void onLayerTextChanged(const uint index, QString text);
    void onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB);
    void onSelectedLayerChanged(const uint index);    

private slots:
    void on_btn_merge_clicked();
    void on_btn_add_clicked();
    void on_btn_moveUp_clicked();
    void on_btn_moveDown_clicked();

    void onDelete(QListWidgetItem* pListWidgetItem);
    void onEnabledChanged(QListWidgetItem* pListWidgetItem, const bool enabled);
    void onTextChanged(QListWidgetItem* pListWidgetItem, QString text);

    void currentRowChanged(int currentRow);

private:
    Ui::DLG_Layers *ui;

    void addLayer(CanvasLayerInfo info);
    void setSelectedLayer(uint index);
};

#endif // DLG_LAYERS_H
