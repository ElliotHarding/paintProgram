#ifndef DLG_COLORMULTIPLIERS_H
#define DLG_COLORMULTIPLIERS_H

#include <QDialog>

namespace Ui {
class DLG_ColorMultipliers;
}

class DLG_ColorMultipliers : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_ColorMultipliers(QWidget *parent = nullptr);
    ~DLG_ColorMultipliers();

    void show();

signals:
    void onColorMultipliers(const int redXred, const int redXgreen, const int redXblue,
                            const int greenXred, const int greenXgreen, const int greenXblue,
                            const int blueXred, const int blueXgreen, const int blueXblue,
                            const int xTransparent);
    void confirmEffects();
    void cancelEffects();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

    void on_spinBox_redXred_valueChanged(int value);
    void on_spinBox_redXgreen_valueChanged(int value);
    void on_spinBox_redXblue_valueChanged(int value);
    void on_spinBox_greenXred_valueChanged(int value);
    void on_spinBox_greenXgreen_valueChanged(int value);
    void on_spinBox_greenXblue_valueChanged(int value);
    void on_spinBox_blueXred_valueChanged(int value);
    void on_spinBox_blueXgreen_valueChanged(int value);
    void on_spinBox_blueXblue_valueChanged(int value);
    void on_spinBox_xAlpha_valueChanged(int value);

    void on_checkBox_boom_toggled(bool checked);
    void on_checkBox_sepia_toggled(bool checked);

private:
    Ui::DLG_ColorMultipliers *ui;

    void closeEvent(QCloseEvent* e) override;

    void reset();
    void clearCheckBoxes();

    void blockSpinBoxSignals(const bool block);

    void applyMultipliers();
};

#endif // DLG_COLORMULTIPLIERS_H
