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
    void onBrightness(const int value);
    void onContrast(const int value);
    void onRedLimit(const int value);
    void onGreenLimit(const int value);
    void onBlueLimit(const int value);
    void onNormalBlur(const int value);

    void confirmEffects();
    void cancelEffects();

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
    void on_slider_hue_valueChanged(int value);
    void on_spinBox_hue_valueChanged(int arg1);
    void on_slider_saturation_valueChanged(int value);
    void on_spinBox_saturation_valueChanged(int arg1);

    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::DLG_EffectsSliders *ui;

    void resetValues();

    void closeEvent(QCloseEvent* e) override;
};

#endif // DLG_EFFECTSSLIDERS_H
