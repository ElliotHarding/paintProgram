#ifndef DLG_SIZE_H
#define DLG_SIZE_H

#include <QDialog>

namespace Ui {
class DLG_Size;
}

class DLG_Size : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Size(QWidget *parent = nullptr);
    ~DLG_Size();

signals:
    void confirmedSize(int width, int height);

private slots:
    void on_btn_okay_clicked();

private:
    Ui::DLG_Size *ui;
};

#endif // DLG_SIZE_H
