#include "dlg_size.h"
#include "ui_dlg_size.h"

DLG_Size::DLG_Size(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Size)
{
    ui->setupUi(this);
}

DLG_Size::~DLG_Size()
{
    delete ui;
}

void DLG_Size::on_btn_okay_clicked()
{
    emit confirmedSize(ui->spinBox_width->value(), ui->spinBox_height->value());
    hide();
}
