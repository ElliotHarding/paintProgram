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

signals:
    void onBrightness(int value);
    void onContrast(int value);

private slots:
    void on_slider_brightness_sliderMoved(int value);
    void on_spinBox_brightness_valueChanged(int value);

private:
    Ui::DLG_EffectsSliders *ui;
};

#endif // DLG_EFFECTSSLIDERS_H
