#include "dlg_tools.h"
#include "ui_dlg_tools.h"

DLG_Tools::DLG_Tools(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dlg_tools)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);//Qt::FramelessWindowHint | Qt::Dialog
    setFixedWidth(74);
    setFixedHeight(158);
}

DLG_Tools::~DLG_Tools()
{
    delete ui;
}

void DLG_Tools::setCurrentTool(Tool tool)
{
    m_currentTool = tool;
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
