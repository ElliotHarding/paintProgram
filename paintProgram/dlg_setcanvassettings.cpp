#include "dlg_setcanvassettings.h"
#include "ui_dlg_setcanvassettings.h"

DLG_SetCanvasSettings::DLG_SetCanvasSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_SetCanvasSettings)
{
    ui->setupUi(this);
}

DLG_SetCanvasSettings::~DLG_SetCanvasSettings()
{
    delete ui;
}

void DLG_SetCanvasSettings::on_btn_okay_clicked()
{
    QString name = ui->textEdit_name->text();
    if(name == "")
        name = "New";

    emit confirmCanvasSettings(ui->spinBox_width->value(), ui->spinBox_height->value(), name);

    hide();
}
