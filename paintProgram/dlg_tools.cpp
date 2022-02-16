#include "dlg_tools.h"
#include "ui_dlg_tools.h"

DLG_Tools::DLG_Tools(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dlg_tools)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    setFixedWidth(64);
    setFixedHeight(195);

    m_currentTool == TOOL_PAINT;
    ui->btn_paintTool->setFlat(false);
}

DLG_Tools::~DLG_Tools()
{
    delete ui;
}

void DLG_Tools::setCurrentTool(const Tool tool)
{
    m_currentTool = tool;

    ui->btn_bucketTool->setFlat(tool!=TOOL_BUCKET);
    ui->btn_colorPickerTool->setFlat(tool!=TOOL_COLOR_PICKER);
    ui->btn_dragTool->setFlat(tool!=TOOL_DRAG);
    ui->btn_eraserTool->setFlat(tool!=TOOL_ERASER);
    ui->btn_paintTool->setFlat(tool!=TOOL_PAINT);
    ui->btn_panTool->setFlat(tool!=TOOL_PAN);
    ui->btn_selectSpreadTool->setFlat(tool!=TOOL_SPREAD_ON_SIMILAR);
    ui->btn_selectTool->setFlat(tool!=TOOL_SELECT);
    ui->btn_shapeTool->setFlat(tool!=TOOL_SHAPE);
    ui->btn_textTool->setFlat(tool!=TOOL_TEXT);

    emit currentToolUpdated(tool);
}

void DLG_Tools::on_btn_selectTool_clicked()
{
    setCurrentTool(TOOL_SELECT);
}

void DLG_Tools::on_btn_paintTool_clicked()
{
    setCurrentTool(TOOL_PAINT);
}

void DLG_Tools::on_btn_selectSpreadTool_clicked()
{
    setCurrentTool(TOOL_SPREAD_ON_SIMILAR);
}

void DLG_Tools::on_btn_eraserTool_clicked()
{
    setCurrentTool(TOOL_ERASER);
}

void DLG_Tools::on_btn_dragTool_clicked()
{
    setCurrentTool(TOOL_DRAG);
}

void DLG_Tools::on_btn_bucketTool_clicked()
{
    setCurrentTool(TOOL_BUCKET);
}

void DLG_Tools::on_btn_colorPickerTool_clicked()
{
    setCurrentTool(TOOL_COLOR_PICKER);
}

void DLG_Tools::on_btn_panTool_clicked()
{
    setCurrentTool(TOOL_PAN);
}

void DLG_Tools::on_btn_textTool_clicked()
{
    setCurrentTool(TOOL_TEXT);
}

void DLG_Tools::on_btn_shapeTool_clicked()
{
    setCurrentTool(TOOL_SHAPE);
}
