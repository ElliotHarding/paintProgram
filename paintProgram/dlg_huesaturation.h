#ifndef DLG_HUESATURATION_H
#define DLG_HUESATURATION_H

#include <QDialog>

namespace Ui {
class DLG_HueSaturation;
}

class DLG_HueSaturation : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_HueSaturation(QWidget *parent = nullptr);
    ~DLG_HueSaturation();

    void show();

signals:
    void onHueSaturation(const int hue, const int saturation);

    void confirmEffects();
    void cancelEffects();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

    void on_slider_hue_valueChanged(int value);
    void on_spinBox_hue_valueChanged(int arg1);

    void on_slider_saturation_valueChanged(int value);
    void on_spinBox_saturation_valueChanged(int arg1);

private:
    Ui::DLG_HueSaturation *ui;

    void closeEvent(QCloseEvent *e) override;

    void reset();
};

#endif // DLG_HUESATURATION_H
