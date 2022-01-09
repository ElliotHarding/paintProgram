#include "dlg_brushsettings.h"
#include "ui_dlg_brushsettings.h"

DLG_BrushSettings::DLG_BrushSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BrushSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_BrushSettings::~DLG_BrushSettings()
{
    delete ui;
}

int DLG_BrushSettings::getBrushSize()
{
    ui->spin_brushSize->value();
}
