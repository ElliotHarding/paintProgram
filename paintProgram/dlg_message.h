#ifndef DLG_MESSAGE_H
#define DLG_MESSAGE_H

#include <QDialog>

namespace Ui {
class DLG_Message;
}

class DLG_Message : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Message(QWidget *parent = nullptr);
    ~DLG_Message();

    void show(QString message);

private slots:
    void on_btn_okay_clicked();

private:
    Ui::DLG_Message *ui;
};

#endif // DLG_MESSAGE_H
