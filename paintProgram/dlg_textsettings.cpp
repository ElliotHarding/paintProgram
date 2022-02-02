#include "dlg_textsettings.h"
#include "ui_dlg_textsettings.h"

DLG_TextSettings::DLG_TextSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_TextSettings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_TextSettings::~DLG_TextSettings()
{
    delete ui;
}

QFont DLG_TextSettings::getFont()
{
    return m_font;
}

void DLG_TextSettings::show()
{
    /*
    m_font.setBold(false);
    ui->btn_bold->setStyleSheet("font : normal;");

    m_font.setItalic(false);
    ui->btn_italic->setStyleSheet("font : normal");

    //No stylesheet for underline
    m_font.setUnderline(false);
    QFont underlineBtnFont = ui->btn_underlined->font();
    underlineBtnFont.setUnderline(false);
    ui->btn_underlined->setFont(underlineBtnFont);*/

    QDialog::show();
}

bool DLG_TextSettings::spinBoxHasFocus()
{
    return ui->spinBox_fontSize->hasFocus();
}

void DLG_TextSettings::on_btn_bold_clicked()
{
    m_font.setBold(!m_font.bold());
    ui->btn_bold->setStyleSheet(m_font.bold() ? "font: bold;" : "font : normal;");

    emit updateFont(m_font);
}

void DLG_TextSettings::on_btn_underlined_clicked()
{
    m_font.setUnderline(!m_font.underline());

    //No stylesheet for underline
    QFont underlineBtnFont = ui->btn_underlined->font();
    underlineBtnFont.setUnderline(m_font.underline());
    ui->btn_underlined->setFont(underlineBtnFont);

    emit updateFont(m_font);
}

void DLG_TextSettings::on_btn_italic_clicked()
{
    m_font.setItalic(!m_font.italic());
    ui->btn_italic->setStyleSheet(m_font.italic() ? "font: italic;" : "font : normal");

    emit updateFont(m_font);
}

void DLG_TextSettings::on_spinBox_fontSize_valueChanged(int size)
{
    m_font.setPointSize(size);
    emit updateFont(m_font);
}
