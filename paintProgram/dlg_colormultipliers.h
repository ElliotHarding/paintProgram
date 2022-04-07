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

    void on_spinBox_redXred_valueChanged(int arg1);

private:
    Ui::DLG_ColorMultipliers *ui;

    void closeEvent(QCloseEvent* e) override;

    void reset();
    void clearCheckBoxes();

    void applyMultipliers();
};

#endif // DLG_COLORMULTIPLIERS_H
