#ifndef DLG_SENSITIVITY_H
#define DLG_SENSITIVITY_H

#include <QDialog>

namespace Ui {
class DLG_Sensitivity;
}

class DLG_Sensitivity : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Sensitivity(QWidget *parent = nullptr);
    ~DLG_Sensitivity();

    int getSensitivity();

private:
    Ui::DLG_Sensitivity *ui;
};

#endif // DLG_SENSITIVITY_H
