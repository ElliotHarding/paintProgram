#include "dlg_blursettings.h"
#include "ui_dlg_blursettings.h"

DLG_BlurSettings::DLG_BlurSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BlurSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_BlurSettings::~DLG_BlurSettings()
{
    delete ui;
}

void DLG_BlurSettings::show()
{
    resetValues();
    QDialog::show();
}

void DLG_BlurSettings::hide()
{
    emit cancelEffects();
    QDialog::hide();
}

void DLG_BlurSettings::closeEvent(QCloseEvent* e)
{
    emit cancelEffects();
    QDialog::closeEvent(e);
}

void DLG_BlurSettings::on_btn_ok_clicked()
{
    emit confirmEffects();
    hide();
}

void DLG_BlurSettings::on_btn_cancel_clicked()
{
    emit cancelEffects();
    hide();
}

void DLG_BlurSettings::on_checkBox_blurTransparent_stateChanged(int activeState)
{
    Q_UNUSED(activeState);
    emit onNormalBlur(ui->spinBox_blurDifference->value(), ui->spinBox_blurAverageArea->value(), ui->checkBox_blurTransparent->checkState() == Qt::CheckState::Checked);
}

void DLG_BlurSettings::on_spinBox_blurDifference_valueChanged(int value)
{
    emit onNormalBlur(value, ui->spinBox_blurAverageArea->value(), ui->checkBox_blurTransparent->checkState() == Qt::CheckState::Checked);
}

void DLG_BlurSettings::on_spinBox_blurAverageArea_valueChanged(int value)
{
    emit onNormalBlur(ui->spinBox_blurDifference->value(), value, ui->checkBox_blurTransparent->checkState() == Qt::CheckState::Checked);
}

void DLG_BlurSettings::resetValues()
{
    ui->spinBox_blurDifference->setValue(0);
    ui->spinBox_blurAverageArea->setValue(0);
    ui->checkBox_blurTransparent->setChecked(true);
}
