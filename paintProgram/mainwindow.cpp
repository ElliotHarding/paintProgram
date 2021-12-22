#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Canvas.h"

#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->c_tabWidget->clear();

    //Will move this code eventually..
    Canvas* c = new Canvas(this, 100, 100);
    ui->c_tabWidget->addTab(c, "My first tab");

    m_colorPicker = new QColorDialog(this);
    m_colorPicker->show();

    //Connections
    connect(ui->actionColor_Picker, SIGNAL(triggered()), this, SLOT(on_open_color_picker()));
}

MainWindow::~MainWindow()
{
    m_colorPicker->close();
    delete m_colorPicker;
    delete ui;
}

QColor MainWindow::getSelectedColor()
{
    return m_colorPicker->currentColor();
}

int MainWindow::getBrushSize()
{
    return ui->spin_brushSize->value();
}

bool MainWindow::isCtrlPressed()
{
    return m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    m_pressedKeys.insert(event->key());
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(m_pressedKeys.find(Qt::Key_C) != m_pressedKeys.end() && m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->copyKeysPressed();
        }
    }

    if(m_pressedKeys.find(Qt::Key_Delete) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->deleteKeyPressed();
        }
    }

    m_pressedKeys.remove(event->key());
}

void MainWindow::on_open_color_picker()
{
    m_colorPicker->show();
}

void MainWindow::on_btn_selectTool_clicked()
{
    setCurrentTool(TOOL_SELECT);
}

void MainWindow::on_btn_paintTool_clicked()
{
    setCurrentTool(TOOL_PAINT);
}

void MainWindow::setCurrentTool(Tool t)
{
    //Loop through all tab widgets controls, if a canvas, update the current tool
    for(int i = 0; i < ui->c_tabWidget->count(); i++)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->widget(i));
        if(c)
        {
            c->setCurrentTool(t);
        }
    }
}

void MainWindow::on_btn_addTab_clicked()
{
    Canvas* c = new Canvas(this, 100, 100); //todo set to dynamic size
    ui->c_tabWidget->addTab(c, "todo");
}

void MainWindow::on_btn_removeTab_clicked()
{
    ui->c_tabWidget->removeTab(ui->c_tabWidget->currentIndex());
}

void MainWindow::on_btn_selectSpreadTool_clicked()
{
    setCurrentTool(TOOL_SPREAD_ON_SIMILAR);
}

void MainWindow::on_btn_eraserTool_clicked()
{
    setCurrentTool(TOOL_ERASER);
}

void MainWindow::on_btn_panTool_clicked()
{
    setCurrentTool(TOOL_PAN);
}

void MainWindow::on_btn_undo_clicked()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->undoPressed();
    }
}

void MainWindow::on_btn_redo_clicked()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->redoPressed();
    }
}
