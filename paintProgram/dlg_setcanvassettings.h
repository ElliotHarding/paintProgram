#ifndef DLG_SETCANVASSETTINGS_H
#define DLG_SETCANVASSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_SetCanvasSettings;
}

class DLG_SetCanvasSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_SetCanvasSettings(QWidget *parent = nullptr);
    ~DLG_SetCanvasSettings();

signals:
    void confirmCanvasSettings(int width, int height, QString name);

private slots:
    void on_btn_okay_clicked();

private:
    Ui::DLG_SetCanvasSettings *ui;
};

#endif // DLG_SETCANVASSETTINGS_H
