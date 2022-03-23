#include "dlg_effectssliders.h"
#include "ui_dlg_effectssliders.h"

DLG_EffectsSliders::DLG_EffectsSliders(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_EffectsSliders)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    resetValues();
}

DLG_EffectsSliders::~DLG_EffectsSliders()
{
    delete ui;
}

void DLG_EffectsSliders::resetValues()
{
    ui->spinBox_brightness->setValue(0);
    ui->slider_brightness->setValue(0);
    ui->spinBox_contrast->setValue(0);
    ui->slider_contrast->setValue(0);
    ui->spinBox_redLimit->setValue(255);
    ui->slider_redLimit->setValue(255);
    ui->spinBox_greenLimit->setValue(255);
    ui->slider_greenLimit->setValue(255);
    ui->spinBox_blueLimit->setValue(255);
    ui->slider_blueLimit->setValue(255);
}

void DLG_EffectsSliders::show()
{
    resetValues();
    QDialog::show();
}

void DLG_EffectsSliders::on_slider_brightness_sliderMoved(int value)
{
    ui->spinBox_brightness->setValue(value);
    emit onBrightness(value);
}

void DLG_EffectsSliders::on_spinBox_brightness_valueChanged(int value)
{
    ui->slider_brightness->setValue(value);
    emit onBrightness(value);
}

void DLG_EffectsSliders::on_slider_contrast_valueChanged(int value)
{
    ui->spinBox_contrast->setValue(value);
    emit onContrast(value);
}

void DLG_EffectsSliders::on_spinBox_contrast_valueChanged(int value)
{
    ui->slider_contrast->setValue(value);
    emit onContrast(value);
}

void DLG_EffectsSliders::on_slider_redLimit_valueChanged(int value)
{
    ui->spinBox_redLimit->setValue(value);
    emit onRedLimit(value);
}

void DLG_EffectsSliders::on_spinBox_redLimit_valueChanged(int value)
{
    ui->slider_redLimit->setValue(value);
    emit onRedLimit(value);
}

void DLG_EffectsSliders::on_slider_greenLimit_valueChanged(int value)
{
    ui->spinBox_greenLimit->setValue(value);
    emit onGreenLimit(value);
}

void DLG_EffectsSliders::on_spinBox_greenLimit_valueChanged(int value)
{
    ui->slider_greenLimit->setValue(value);
    emit onGreenLimit(value);
}

void DLG_EffectsSliders::on_slider_blueLimit_valueChanged(int value)
{
    ui->spinBox_blueLimit->setValue(value);
    emit onBlueLimit(value);
}

void DLG_EffectsSliders::on_spinBox_blueLimit_valueChanged(int value)
{
    ui->slider_blueLimit->setValue(value);
    emit onBlueLimit(value);
}

void DLG_EffectsSliders::on_spinBox_normalBlur_valueChanged(int value)
{
    ui->slider_normalBlur->setValue(value);
    emit onNormalBlur(value);
}

void DLG_EffectsSliders::on_slider_normalBlur_valueChanged(int value)
{
    ui->spinBox_normalBlur->setValue(value);
    emit onNormalBlur(value);
}

void DLG_EffectsSliders::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_EffectsSliders::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}


