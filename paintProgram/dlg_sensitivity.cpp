#include "dlg_sensitivity.h"
#include "ui_dlg_sensitivity.h"

DLG_Sensitivity::DLG_Sensitivity(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Sensitivity)
{
    ui->setupUi(this);
}

DLG_Sensitivity::~DLG_Sensitivity()
{
    delete ui;
}

int DLG_Sensitivity::getSensitivity()
{
    return ui->spin_spreadSensitivity->value();
}
