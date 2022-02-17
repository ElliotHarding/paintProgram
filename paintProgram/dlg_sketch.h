#ifndef DLG_SKETCH_H
#define DLG_SKETCH_H

#include <QDialog>

namespace Ui {
class DLG_Sketch;
}

class DLG_Sketch : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Sketch(QWidget *parent = nullptr);
    ~DLG_Sketch();

    void show();

signals:
    void onOutlineEffect(const int value);
    void onSketchEffect(const int value);

    void confirmEffects();
    void cancelEffects();

private slots:

    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

    void on_slider_outline_valueChanged(int value);
    void on_spinBox_outline_valueChanged(int value);
    void on_slider_sketch_valueChanged(int value);
    void on_spinBox_sketch_valueChanged(int value);

private:
    Ui::DLG_Sketch *ui;

    void reset();
};

#endif // DLG_SKETCH_H
