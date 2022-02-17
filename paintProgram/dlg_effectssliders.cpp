#include "dlg_effectssliders.h"
#include "ui_dlg_effectssliders.h"

DLG_EffectsSliders::DLG_EffectsSliders(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_EffectsSliders)
{
    ui->setupUi(this);
}

DLG_EffectsSliders::~DLG_EffectsSliders()
{
    delete ui;
}

void DLG_EffectsSliders::on_slider_brightness_sliderMoved(int value)
{
    ui->spinBox_brightness->setValue(position);
}

void DLG_EffectsSliders::on_spinBox_brightness_valueChanged(int value)
{
    ui->slider_brightness->setValue(value);
}

void DLG_EffectsSliders::on_slider_contrast_valueChanged(int value)
{

}

void DLG_EffectsSliders::on_spinBox_contrast_valueChanged(int arg1)
{

}

void DLG_EffectsSliders::on_slider_redLimit_valueChanged(int value)
{

}

void DLG_EffectsSliders::on_spinBox_redLimit_valueChanged(int arg1)
{

}

void DLG_EffectsSliders::on_slider_greenLimit_valueChanged(int value)
{

}

void DLG_EffectsSliders::on_spinBox_greenLimit_valueChanged(int arg1)
{

}

void DLG_EffectsSliders::on_slider_blueLimit_valueChanged(int value)
{

}

void DLG_EffectsSliders::on_spinBox_blueLimit_valueChanged(int arg1)
{

}

void DLG_EffectsSliders::on_btn_ok_clicked()
{
    hide();
}
