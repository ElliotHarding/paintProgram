#include "dlg_setcanvassettings.h"
#include "ui_dlg_setcanvassettings.h"

DLG_SetCanvasSettings::DLG_SetCanvasSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_SetCanvasSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_SetCanvasSettings::~DLG_SetCanvasSettings()
{
    delete ui;
}

void DLG_SetCanvasSettings::setCurrentValues(int width, int height, QString name)
{
    ui->spinBox_width->setValue(width);
    ui->spinBox_height->setValue(height);
    ui->textEdit_name->setText(name);

    update();
}

void DLG_SetCanvasSettings::on_btn_okay_clicked()
{
    QString name = ui->textEdit_name->text();
    if(name == "")
        name = "New";

    emit confirmCanvasSettings(ui->spinBox_width->value(), ui->spinBox_height->value(), name);

    hide();
}

void DLG_SetCanvasSettings::on_btn_cancel_clicked()
{
    hide();
}
