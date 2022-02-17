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
