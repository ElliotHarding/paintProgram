#ifndef DLG_TEXTSETTINGS_H
#define DLG_TEXTSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_TextSettings;
}

class DLG_TextSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_TextSettings(QWidget *parent = nullptr);
    ~DLG_TextSettings();

    QFont getFont();
    void show();

    bool spinBoxHasFocus();

signals:
    void fontUpdated();

private slots:
    void on_btn_bold_clicked();
    void on_btn_underlined_clicked();
    void on_btn_italic_clicked();
    void on_spinBox_fontSize_valueChanged(int arg1);

private:
    Ui::DLG_TextSettings *ui;

    QFont m_font;
};

#endif // DLG_TEXTSETTINGS_H
