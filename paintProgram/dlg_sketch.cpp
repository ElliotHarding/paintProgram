#include "dlg_sketch.h"
#include "ui_dlg_sketch.h"

DLG_Sketch::DLG_Sketch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Sketch)
{
    ui->setupUi(this);
}

DLG_Sketch::~DLG_Sketch()
{
    delete ui;
}
