#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Canvas.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Canvas* c = new Canvas(this, 100, 100);
    ui->c_tabWidget->clear();
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

void MainWindow::on_open_color_picker()
{
    m_colorPicker->show();
}

