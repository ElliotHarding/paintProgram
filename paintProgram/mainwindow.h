#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColorDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum Tool
{
    TOOL_PAINT,
    TOOL_SELECT,
    TOOL_SPREAD_ON_SIMILAR
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QColor getSelectedColor();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_open_color_picker();
    void on_btn_selectTool_clicked();
    void on_btn_paintTool_clicked();
    void on_btn_addTab_clicked();
    void on_btn_removeTab_clicked();

private:
    Ui::MainWindow *ui;

    QColorDialog* m_colorPicker;

    Tool m_currentTool = TOOL_PAINT;
    void setCurrentTool(Tool t);
};
#endif // MAINWINDOW_H
