#ifndef DLG_LAYERS_H
#define DLG_LAYERS_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class DLG_Layers;
}

class DLG_Layers : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Layers(QWidget *parent = nullptr);
    ~DLG_Layers();

signals:
    void onLayerAdded(const uint id);
    void onLayerDeleted(const uint id);
    void onLayerEnabledChanged(const uint id, const bool enabled);

private slots:
    void on_btn_merge_clicked();
    void on_btn_add_clicked();

    void onDelete(QListWidgetItem* pListWidgetItem);
    void onEnabledChanged(QListWidgetItem* pListWidgetItem, const bool enabled);

private:
    Ui::DLG_Layers *ui;
};

#endif // DLG_LAYERS_H
