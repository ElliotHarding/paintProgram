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

    newCanvas(200, 200);

    m_colorPicker = new QColorDialog(this);
    m_colorPicker->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel);
    m_colorPicker->show();

    m_dlg_size = new DLG_Size();

    //Connections
    connect(m_dlg_size, SIGNAL(confirmedSize(int,int)), this, SLOT(newCanvas(int,int)));
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

int MainWindow::getSpreadSensitivity()
{
    return ui->spin_spreadSensitivity->value();
}

void MainWindow::setCopyBuffer(QImage image)
{
    m_copyBuffer = image;
}

QImage MainWindow::getCopyBuffer()
{
    return m_copyBuffer;
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

    if(m_pressedKeys.find(Qt::Key_V) != m_pressedKeys.end() && m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->pasteKeysPressed();
        }
    }

    if(m_pressedKeys.find(Qt::Key_X) != m_pressedKeys.end() && m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->copyKeysPressed();
            c->deleteKeyPressed();
        }
    }

    m_pressedKeys.remove(event->key());
}

void MainWindow::newCanvas(int width, int height)
{
    Canvas* c = new Canvas(this, width, height);

    c->updateCurrentTool(m_currentTool);
    connect(this, SIGNAL(updateCurrentTool(Tool)), c, SLOT(updateCurrentTool(Tool)));

    ui->c_tabWidget->addTab(c, "todo");
}

void MainWindow::on_open_color_picker()
{
    m_colorPicker->show();
}

void MainWindow::on_btn_addTab_clicked()
{
    m_dlg_size->show();
}

void MainWindow::on_btn_removeTab_clicked()
{
    ui->c_tabWidget->removeTab(ui->c_tabWidget->currentIndex());
}

void MainWindow::setCurrentTool(Tool t)
{
    m_currentTool = t;
    emit updateCurrentTool(t);
}

void MainWindow::on_btn_selectTool_clicked()
{
    setCurrentTool(TOOL_SELECT);
}

void MainWindow::on_btn_paintTool_clicked()
{
    setCurrentTool(TOOL_PAINT);
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

void MainWindow::on_btn_dragTool_clicked()
{
    setCurrentTool(TOOL_DRAG);
}

void MainWindow::on_btn_bucketTool_clicked()
{
    setCurrentTool(TOOL_BUCKET);
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
