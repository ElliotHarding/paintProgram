#include "dlg_brushsettings.h"
#include "ui_dlg_brushsettings.h"
#include <QDebug>

DLG_BrushSettings::DLG_BrushSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_BrushSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    setFixedWidth(139);

    ui->btn_shapeCircle->setFlat(true);
    ui->btn_shapeRect->setFlat(false);
    m_brushShape = BRUSHSHAPE_RECT;
}

DLG_BrushSettings::~DLG_BrushSettings()
{
    delete ui;
}

int DLG_BrushSettings::getBrushSize()
{
    return ui->spin_brushSize->value();
}

BrushShape DLG_BrushSettings::getBrushShape()
{
    return m_brushShape;
}

void DLG_BrushSettings::on_btn_shapeRect_clicked()
{
    ui->btn_shapeCircle->setFlat(true);
    ui->btn_shapeRect->setFlat(false);
    m_brushShape = BRUSHSHAPE_RECT;
}

void DLG_BrushSettings::on_btn_shapeCircle_clicked()
{
    ui->btn_shapeCircle->setFlat(false);
    ui->btn_shapeRect->setFlat(true);
    m_brushShape = BRUSHSHAPE_CIRCLE;
}
