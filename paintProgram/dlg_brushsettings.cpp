#include "dlg_brushsettings.h"
#include "ui_dlg_brushsettings.h"
#include <QDebug>

DLG_BrushSettings::DLG_BrushSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BrushSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    setFixedWidth(70);
}

DLG_BrushSettings::~DLG_BrushSettings()
{
    delete ui;
}

int DLG_BrushSettings::getBrushSize()
{
    return ui->spin_brushSize->value();
}
