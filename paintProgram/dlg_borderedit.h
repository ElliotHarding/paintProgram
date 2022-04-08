#ifndef DLG_BORDEREDIT_H
#define DLG_BORDEREDIT_H

#include <QDialog>

namespace Ui {
class DLG_BorderEdit;
}

class DLG_BorderEdit : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_BorderEdit(QWidget *parent = nullptr);
    ~DLG_BorderEdit();

    void show();

signals:
    void onBorderEdit(const int borderEdges, const bool includeCorners, const bool removeCenter);

    void confirmEffects();
    void cancelEffects();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

    void on_spinBox_borderThickness_valueChanged(int value);
    void on_checkBox_includeCorners_stateChanged(int value);
    void on_checkBox_removeCenter_stateChanged(int value);

private:
    Ui::DLG_BorderEdit *ui;

    void closeEvent(QCloseEvent* e) override;

    void reset();
};

#endif // DLG_BORDEREDIT_H
