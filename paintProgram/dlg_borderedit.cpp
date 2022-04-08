#include "dlg_borderedit.h"
#include "ui_dlg_borderedit.h"

DLG_BorderEdit::DLG_BorderEdit(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BorderEdit)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_BorderEdit::~DLG_BorderEdit()
{
    delete ui;
}

void DLG_BorderEdit::show()
{
    reset();
    QDialog::show();
}

void DLG_BorderEdit::closeEvent(QCloseEvent* e)
{
    emit cancelEffects();
    QDialog::closeEvent(e);
}

void DLG_BorderEdit::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_BorderEdit::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}

void DLG_BorderEdit::on_spinBox_borderThickness_valueChanged(int value)
{
    emit onBorderEdit(value, ui->checkBox_includeCorners->isChecked(), ui->checkBox_removeCenter->isChecked());
}

void DLG_BorderEdit::on_checkBox_includeCorners_stateChanged(int value)
{
    emit onBorderEdit(ui->spinBox_borderThickness->value(), value, ui->checkBox_removeCenter->isChecked());
}

void DLG_BorderEdit::on_checkBox_removeCenter_stateChanged(int value)
{
    emit onBorderEdit(ui->spinBox_borderThickness->value(), ui->checkBox_includeCorners->isChecked(), value);
}

void DLG_BorderEdit::reset()
{
    ui->spinBox_borderThickness->setValue(0);
    ui->checkBox_includeCorners->setChecked(false);
    ui->checkBox_removeCenter->setChecked(false);
}
