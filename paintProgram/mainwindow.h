#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//todo ~ colorPicker ~ shapes ~ text

#include <QMainWindow>
#include <QColorDialog>
#include <QSet>

#include "dlg_setcanvassettings.h"
#include "dlg_tools.h"
#include "dlg_textsettings.h"
#include "dlg_brushsettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Canvas;

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

    QFont getTextFont();

protected: //todo - can remove the key events because event filter handles them....
    bool eventFilter(QObject* watched, QEvent* event ) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* moveEvent) override;

private slots:
    void on_get_canvas_settings(int width, int height, QString name);

    void updatedCurrentTool(Tool tool);

    void on_update_font(QFont font);

    void on_open_color_picker();
    void on_open_tools();

    void on_load();
    void on_save();
    void on_save_as();

    void on_btn_addTab_clicked();

    void on_btn_undo_clicked();
    void on_btn_redo_clicked();

    void on_c_tabWidget_tabCloseRequested(int index);

    void on_btn_canvasSettings_clicked();

private:
    Ui::MainWindow *ui;

    //Dialogs
    DLG_SetCanvasSettings* m_dlg_canvasSettings = nullptr;
    DLG_Tools* m_dlg_tools = nullptr;
    DLG_TextSettings* m_dlg_textSettings = nullptr;
    DLG_BrushSettings* m_dlg_brushSettings = nullptr;
    QColorDialog* m_dlg_colorPicker = nullptr;

    QSet<int> m_pressedKeys;

    QImage m_copyBuffer;

    void loadNewCanvas(QImage image, QString name, QString savePath = "");
    bool m_bMakingNewCanvas = false;

    QString getSaveAsPath(QString name);
    void saveCanvas(Canvas* canvas, QString path);
};
#endif // MAINWINDOW_H
