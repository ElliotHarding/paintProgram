#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Canvas.h"

#include <QKeyEvent>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->c_tabWidget->clear();

    m_dlg_colorPicker = new QColorDialog(this);
    m_dlg_colorPicker->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel);
    m_dlg_colorPicker->show();

    m_dlg_size = new DLG_Size(this);

    m_dlg_tools = new DLG_Tools(this);
    m_dlg_tools->show();

    on_new_canvas(200, 200);

    //Connections
    connect(m_dlg_size, SIGNAL(confirmedSize(int,int)), this, SLOT(on_new_canvas(int,int)));
    connect(ui->actionColor_Picker, SIGNAL(triggered()), this, SLOT(on_open_color_picker()));
    connect(ui->actionTools, SIGNAL(triggered()), this, SLOT(on_open_tools()));
    connect(ui->actionLoad_Image, SIGNAL(triggered()), this, SLOT(on_load_image()));
    connect(ui->actionSave_Image, SIGNAL(triggered()), this, SLOT(on_save_image()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

QColor MainWindow::getSelectedColor()
{
    return m_dlg_colorPicker->currentColor();
}

void MainWindow::setSelectedColor(QColor col)
{
    m_dlg_colorPicker->setCurrentColor(col);
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        keyPressEvent(dynamic_cast<QKeyEvent*>(event));
    }
    else if(event->type() == QEvent::KeyRelease)
    {
        keyReleaseEvent(dynamic_cast<QKeyEvent*>(event));
    }
    return QObject::eventFilter( watched, event );
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
            c->cutKeysPressed();
        }
    }

    m_pressedKeys.remove(event->key());
}

void MainWindow::loadNewCanvas(QImage image)
{
    Canvas* c = new Canvas(this, image);

    c->updateCurrentTool(m_dlg_tools->getCurrentTool());
    connect(m_dlg_tools, SIGNAL(currentToolUpdated(Tool)), c, SLOT(updateCurrentTool(Tool)));

    ui->c_tabWidget->addTab(c, "todo");
}

void MainWindow::on_load_image()
{
    QFileDialog loadDialog;
    QString filePath = loadDialog.getOpenFileUrl(this).path();

    //todo ~ why do i need to cut the first / of the url?
    filePath = filePath.mid(1, filePath.length());

    QImage image(filePath);
    loadNewCanvas(image);
}

void MainWindow::on_save_image()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        QImage image = c->getImageCopy();
        qDebug() << image.save(ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()) + ".png");
    }
}

void MainWindow::on_new_canvas(int width, int height)
{
    loadNewCanvas(QImage(QSize(width, height), QImage::Format_ARGB32));
}

void MainWindow::on_open_color_picker()
{
    m_dlg_colorPicker->show();
}

void MainWindow::on_open_tools()
{
    m_dlg_tools->show();
}

void MainWindow::on_btn_addTab_clicked()
{
    m_dlg_size->show();
}

void MainWindow::on_btn_removeTab_clicked()
{
    ui->c_tabWidget->removeTab(ui->c_tabWidget->currentIndex());
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
