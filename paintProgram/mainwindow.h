#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//todo ~ colorPicker ~ shapes ~ text

#include <QMainWindow>
#include <QColorDialog>
#include <QSet>

#include "dlg_size.h"
#include "dlg_tools.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QColor getSelectedColor();
    void setSelectedColor(QColor col);

    int getBrushSize();

    int getSpreadSensitivity();

    bool isCtrlPressed();

    void setCopyBuffer(QImage image);
    QImage getCopyBuffer();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_new_canvas(int width, int height);

    void on_open_color_picker();
    void on_open_tools();

    void on_load_image();
    void on_save_image();

    void on_btn_addTab_clicked();
    void on_btn_removeTab_clicked();

    void on_btn_undo_clicked();
    void on_btn_redo_clicked();

private:
    Ui::MainWindow *ui;

    //Dialogs
    DLG_Size* m_dlg_size;
    DLG_Tools* m_dlg_tools;
    QColorDialog* m_dlg_colorPicker;

    QSet<int> m_pressedKeys;

    QImage m_copyBuffer;

    void loadNewCanvas(QImage image);
};
#endif // MAINWINDOW_H
