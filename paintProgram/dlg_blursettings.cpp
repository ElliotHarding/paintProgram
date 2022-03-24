#include "dlg_blursettings.h"
#include "ui_dlg_blursettings.h"

DLG_BlurSettings::DLG_BlurSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BlurSettings)
{
    ui->setupUi(this);
}

DLG_BlurSettings::~DLG_BlurSettings()
{
    delete ui;
}
