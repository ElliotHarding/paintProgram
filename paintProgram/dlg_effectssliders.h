#ifndef DLG_EFFECTSSLIDERS_H
#define DLG_EFFECTSSLIDERS_H

#include <QDialog>

namespace Ui {
class DLG_EffectsSliders;
}

class DLG_EffectsSliders : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_EffectsSliders(QWidget *parent = nullptr);
    ~DLG_EffectsSliders();

    void show();

signals:
    void onBrightness(int value);
    void onContrast(int value);
    void onRedLimit(int value);
    void onGreenLimit(int value);
    void onBlueLimit(int value);

private slots:
    void on_slider_brightness_sliderMoved(int value);
    void on_spinBox_brightness_valueChanged(int value);
    void on_slider_contrast_valueChanged(int value);
    void on_spinBox_contrast_valueChanged(int arg1);
    void on_slider_redLimit_valueChanged(int value);
    void on_spinBox_redLimit_valueChanged(int arg1);
    void on_slider_greenLimit_valueChanged(int value);
    void on_spinBox_greenLimit_valueChanged(int arg1);
    void on_slider_blueLimit_valueChanged(int value);
    void on_spinBox_blueLimit_valueChanged(int arg1);

    void on_btn_ok_clicked();

private:
    Ui::DLG_EffectsSliders *ui;

    void resetValues();
};

#endif // DLG_EFFECTSSLIDERS_H
