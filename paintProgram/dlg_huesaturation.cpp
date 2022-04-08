#include "dlg_huesaturation.h"
#include "ui_dlg_huesaturation.h"

DLG_HueSaturation::DLG_HueSaturation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_HueSaturation)
{
    ui->setupUi(this);
}

DLG_HueSaturation::~DLG_HueSaturation()
{
    delete ui;
}

void DLG_HueSaturation::show()
{
    reset();
    QDialog::show();
}

void DLG_HueSaturation::closeEvent(QCloseEvent *e)
{
    emit cancelEffects();
    QDialog::closeEvent(e);
}

void DLG_HueSaturation::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_HueSaturation::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}

void DLG_HueSaturation::on_slider_hue_valueChanged(int value)
{
    ui->spinBox_hue->blockSignals(true);
    ui->spinBox_hue->setValue(value);
    ui->spinBox_hue->blockSignals(false);
    emit onHueSaturation(value, ui->spinBox_saturation->value());
}

void DLG_HueSaturation::on_spinBox_hue_valueChanged(int value)
{
    ui->slider_hue->blockSignals(true);
    ui->slider_hue->setValue(value);
    ui->slider_hue->blockSignals(false);
    emit onHueSaturation(value, ui->spinBox_saturation->value());
}

void DLG_HueSaturation::on_slider_saturation_valueChanged(int value)
{
    ui->spinBox_saturation->blockSignals(true);
    ui->spinBox_saturation->setValue(value);
    ui->spinBox_saturation->blockSignals(false);
    emit onHueSaturation(ui->spinBox_hue->value(), value);
}

void DLG_HueSaturation::on_spinBox_saturation_valueChanged(int value)
{
    ui->slider_saturation->blockSignals(true);
    ui->slider_saturation->setValue(value);
    ui->slider_saturation->blockSignals(false);
    emit onHueSaturation(ui->spinBox_hue->value(), value);
}

void DLG_HueSaturation::reset()
{
    ui->spinBox_hue->setValue(0);
    ui->slider_hue->setValue(0);
    ui->spinBox_saturation->setValue(0);
    ui->slider_saturation->setValue(0);
}
