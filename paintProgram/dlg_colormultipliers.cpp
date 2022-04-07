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
