#include "dlg_sketch.h"
#include "ui_dlg_sketch.h"

DLG_Sketch::DLG_Sketch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Sketch)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    reset();
}

DLG_Sketch::~DLG_Sketch()
{
    delete ui;
}

void DLG_Sketch::show()
{
    reset();
    QDialog::show();
}

void DLG_Sketch::on_slider_outline_valueChanged(int value)
{
    ui->spinBox_outline->setValue(value);
    emit onOutlineEffect(value);
}

void DLG_Sketch::on_spinBox_outline_valueChanged(int value)
{
    ui->slider_outline->setValue(value);
    emit onOutlineEffect(value);
}

void DLG_Sketch::on_slider_sketch_valueChanged(int value)
{
    ui->spinBox_sketch->setValue(value);
    emit onSketchEffect(value);
}

void DLG_Sketch::on_spinBox_sketch_valueChanged(int value)
{
    ui->slider_sketch->setValue(value);
    emit onSketchEffect(value);
}

void DLG_Sketch::reset()
{
    ui->slider_sketch->setValue(0);
    ui->spinBox_sketch->setValue(0);
    ui->slider_outline->setValue(0);
    ui->spinBox_outline->setValue(0);
}

void DLG_Sketch::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_Sketch::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}
