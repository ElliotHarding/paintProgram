#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//todo ~ colorPicker ~ shapes ~ text

#include <QMainWindow>
#include <QColorDialog>
#include <QSet>

#include "dlg_size.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum Tool
{
    TOOL_PAINT,
    TOOL_SELECT,
    TOOL_SPREAD_ON_SIMILAR,
    TOOL_ERASER,
    TOOL_PAN,
    TOOL_DRAG,
    TOOL_BUCKET,
    TOOL_COLOR_PICKER
};

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

signals:
    void updateCurrentTool(Tool tool);

private slots:
    void on_new_canvas(int width, int height);

    void on_open_color_picker();
    void on_load_image();
    void on_save_image();

    void on_btn_selectTool_clicked();
    void on_btn_paintTool_clicked();
    void on_btn_addTab_clicked();
    void on_btn_removeTab_clicked();
    void on_btn_selectSpreadTool_clicked();
    void on_btn_eraserTool_clicked();
    void on_btn_panTool_clicked();
    void on_btn_undo_clicked();
    void on_btn_redo_clicked();
    void on_btn_dragTool_clicked();
    void on_btn_bucketTool_clicked();
    void on_btn_colorPickerTool_clicked();

private:
    Ui::MainWindow *ui;

    QColorDialog* m_colorPicker;

    DLG_Size* m_dlg_size;

    Tool m_currentTool = TOOL_PAINT;
    void setCurrentTool(Tool t);

    QSet<int> m_pressedKeys;

    QImage m_copyBuffer;

    void loadNewCanvas(QImage image);
};
#endif // MAINWINDOW_H
