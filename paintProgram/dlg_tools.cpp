#include "dlg_tools.h"
#include "ui_dlg_tools.h"

dlg_tools::dlg_tools(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dlg_tools)
{
    ui->setupUi(this);
}

dlg_tools::~dlg_tools()
{
    delete ui;
}
