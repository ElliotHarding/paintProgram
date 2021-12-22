#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColorDialog>
#include <QSet>

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
    TOOL_DRAG
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QColor getSelectedColor();
    int getBrushSize();

    int getSpreadSensitivity();

    bool isCtrlPressed();

    void setCopyBuffer(QImage image);
    QImage getCopyBuffer();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_open_color_picker();

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

private:
    Ui::MainWindow *ui;

    QColorDialog* m_colorPicker;

    Tool m_currentTool = TOOL_PAINT;
    void setCurrentTool(Tool t);

    QSet<int> m_pressedKeys;

    QImage m_copyBuffer;
};
#endif // MAINWINDOW_H
