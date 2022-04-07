#include "dlg_colormultipliers.h"
#include "ui_dlg_colormultipliers.h"

DLG_ColorMultipliers::DLG_ColorMultipliers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_ColorMultipliers)
{
    ui->setupUi(this);
}

DLG_ColorMultipliers::~DLG_ColorMultipliers()
{
    delete ui;
}

void DLG_ColorMultipliers::show()
{
    reset();
    QDialog::show();
}

void DLG_ColorMultipliers::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_ColorMultipliers::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}

void DLG_ColorMultipliers::closeEvent(QCloseEvent *e)
{
    emit cancelEffects();
    QDialog::closeEvent(e);
}

void DLG_ColorMultipliers::reset()
{
    ui->spinBox_redXred->setValue(100);
    ui->spinBox_redXgreen->setValue(0);
    ui->spinBox_redXblue->setValue(0);

    ui->spinBox_greenXred->setValue(0);
    ui->spinBox_greenXgreen->setValue(100);
    ui->spinBox_greenXblue->setValue(0);

    ui->spinBox_blueXred->setValue(0);
    ui->spinBox_blueXgreen->setValue(0);
    ui->spinBox_blueXblue->setValue(100);

    ui->spinBox_xAlpha->setValue(100);

    clearCheckBoxes();
}

void DLG_ColorMultipliers::clearCheckBoxes()
{
    ui->checkBox_sepia->setChecked(false);
    ui->checkBox_boom->setChecked(false);
}

void DLG_ColorMultipliers::applyMultipliers()
{
    emit onColorMultipliers(ui->spinBox_redXred->value(), ui->spinBox_redXgreen->value(), ui->spinBox_redXblue->value(),
                            ui->spinBox_greenXred->value(), ui->spinBox_greenXgreen->value(), ui->spinBox_greenXblue->value(),
                            ui->spinBox_blueXred->value(), ui->spinBox_blueXgreen->value(), ui->spinBox_blueXblue->value(),
                            ui->spinBox_xAlpha->value());
}

void DLG_ColorMultipliers::on_spinBox_redXred_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_redXgreen_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_redXblue_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_greenXred_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_greenXgreen_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_greenXblue_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_blueXred_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_blueXgreen_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_blueXblue_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_spinBox_xAlpha_valueChanged(int value)
{
    Q_UNUSED(value);
    clearCheckBoxes();
    applyMultipliers();
}

void DLG_ColorMultipliers::on_checkBox_boom_toggled(bool checked)
{
    ui->checkBox_sepia->setChecked(false);
    if(checked)
    {
        ui->spinBox_redXred->setValue(20);
        ui->spinBox_redXgreen->setValue(20);
        ui->spinBox_redXblue->setValue(100);

        ui->spinBox_greenXred->setValue(20);
        ui->spinBox_greenXgreen->setValue(20);
        ui->spinBox_greenXblue->setValue(100);

        ui->spinBox_blueXred->setValue(20);
        ui->spinBox_blueXgreen->setValue(20);
        ui->spinBox_blueXblue->setValue(100);

        ui->spinBox_xAlpha->setValue(100);

        applyMultipliers();
    }
}

void DLG_ColorMultipliers::on_checkBox_sepia_toggled(bool checked)
{
    ui->checkBox_boom->setChecked(false);
    if(checked)
    {
        ui->spinBox_redXred->setValue(39);
        ui->spinBox_redXgreen->setValue(76);
        ui->spinBox_redXblue->setValue(18);

        ui->spinBox_greenXred->setValue(34);
        ui->spinBox_greenXgreen->setValue(68);
        ui->spinBox_greenXblue->setValue(16);

        ui->spinBox_blueXred->setValue(27);
        ui->spinBox_blueXgreen->setValue(53);
        ui->spinBox_blueXblue->setValue(13);

        ui->spinBox_xAlpha->setValue(100);

        applyMultipliers();
    }
}
