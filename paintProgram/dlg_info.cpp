#include "dlg_info.h"
#include "ui_dlg_info.h"

DLG_Info::DLG_Info(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Info)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_Info::~DLG_Info()
{
    delete ui;
}

void DLG_Info::onSelectionAreaResize(const int x, const int y)
{
    ui->text_selectionSize->setText(QString::number(x) + ":" + QString::number(y));
}

void DLG_Info::onMousePositionChange(const int x, const int y)
{
    ui->text_mousePosition->setText(QString::number(x) + ":" + QString::number(y));
}

void DLG_Info::onCanvasSizeChange(const int x, const int y)
{
    ui->text_cavasSize->setText(QString::number(x) + "x" + QString::number(y));
}
