#include "dlg_message.h"
#include "ui_dlg_message.h"

DLG_Message::DLG_Message(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Message)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_Message::~DLG_Message()
{
    delete ui;
}

void DLG_Message::show(QString message)
{
    ui->label_message->setText(message);
    QWidget::show();
}

void DLG_Message::on_btn_okay_clicked()
{
    hide();
}
