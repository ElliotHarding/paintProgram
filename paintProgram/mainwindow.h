#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//todo ~ colorPicker ~ shapes ~ text

#include <QMainWindow>
#include <QColorDialog>
#include <QSet>

#include "dlg_setcanvassettings.h"
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

protected: //todo - can remove the key events because event filter handles them....
    bool eventFilter(QObject* watched, QEvent* event ) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_get_canvas_settings(int width, int height, QString name);

    void on_open_color_picker();
    void on_open_tools();

    void on_load_image();
    void on_save_image();

    void on_btn_addTab_clicked();

    void on_btn_undo_clicked();
    void on_btn_redo_clicked();

    void on_c_tabWidget_tabCloseRequested(int index);

private:
    Ui::MainWindow *ui;

    //Dialogs
    DLG_SetCanvasSettings* m_dlg_canvasSettings;
    DLG_Tools* m_dlg_tools;
    QColorDialog* m_dlg_colorPicker;

    QSet<int> m_pressedKeys;

    QImage m_copyBuffer;

    void loadNewCanvas(QImage image);
};
#endif // MAINWINDOW_H
