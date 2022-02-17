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

private slots:
    void on_btn_merge_clicked();
    void on_btn_add_clicked();

    void onDelete(QListWidgetItem* pListWidgetItem);

private:
    Ui::DLG_Layers *ui;
};

#endif // DLG_LAYERS_H
