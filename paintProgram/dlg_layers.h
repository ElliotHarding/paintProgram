#ifndef DLG_LAYERS_H
#define DLG_LAYERS_H

#include <QDialog>

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

private:
    Ui::DLG_Layers *ui;
};

#endif // DLG_LAYERS_H
