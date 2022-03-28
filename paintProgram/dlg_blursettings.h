#ifndef DLG_BLURSETTINGS_H
#define DLG_BLURSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_BlurSettings;
}

class DLG_BlurSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_BlurSettings(QWidget *parent = nullptr);
    ~DLG_BlurSettings();

    void show();

signals:
    void onNormalBlur(const int difference, const int averageArea, const bool includeTransparent);

    void confirmEffects();
    void cancelEffects();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

    void on_checkBox_blurTransparent_stateChanged(int activeState);
    void on_spinBox_blurDifference_valueChanged(int value);
    void on_spinBox_blurAverageArea_valueChanged(int value);

private:
    Ui::DLG_BlurSettings *ui;

    void resetValues();
};

#endif // DLG_BLURSETTINGS_H
