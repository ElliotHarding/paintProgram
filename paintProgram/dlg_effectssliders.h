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

    void confirmEffects();
    void cancelEffects();

private slots:
    void on_slider_brightness_sliderMoved(int value);
    void on_spinBox_brightness_valueChanged(int value);
    void on_slider_contrast_valueChanged(int value);
    void on_spinBox_contrast_valueChanged(int arg1);

    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::DLG_EffectsSliders *ui;

    void resetValues();

    void closeEvent(QCloseEvent* e) override;
};

#endif // DLG_EFFECTSSLIDERS_H
