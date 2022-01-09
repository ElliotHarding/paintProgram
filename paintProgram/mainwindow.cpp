#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Canvas.h"

#include <QDesktopWidget>
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
    m_dlg_colorPicker->setOptions(QColorDialog::ColorDialogOption::ShowAlphaChannel | QColorDialog::ColorDialogOption::NoButtons | QColorDialog::ColorDialogOption::DontUseNativeDialog);
    m_dlg_colorPicker->show();

    m_dlg_canvasSettings = new DLG_SetCanvasSettings(this);

    m_dlg_tools = new DLG_Tools(this);
    m_dlg_tools->show();

    m_dlg_textSettings = new DLG_TextSettings(this);

    m_dlg_brushSettings = new DLG_BrushSettings(this);
    m_dlg_brushSettings->show();

    //Connections
    connect(m_dlg_canvasSettings, SIGNAL(confirmCanvasSettings(int,int,QString)), this, SLOT(on_get_canvas_settings(int,int,QString)));
    connect(ui->actionColor_Picker, SIGNAL(triggered()), this, SLOT(on_open_color_picker()));
    connect(ui->actionTools, SIGNAL(triggered()), this, SLOT(on_open_tools()));
    connect(ui->actionLoad_Image, SIGNAL(triggered()), this, SLOT(on_load()));
    connect(ui->actionSave_Image, SIGNAL(triggered()), this, SLOT(on_save()));
    connect(ui->actionImageSettings, SIGNAL(triggered()), this, SLOT(on_btn_canvasSettings_clicked()));
    connect(ui->actionSave_As, SIGNAL(triggered()), this, SLOT(on_save_as()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(on_btn_addTab_clicked()));
    connect(m_dlg_tools, SIGNAL(currentToolUpdated(Tool)), this, SLOT(updatedCurrentTool(Tool)));
    connect(m_dlg_textSettings, SIGNAL(updateFont(QFont)), this, SLOT(on_update_font(QFont)));

    showMaximized();

    m_bMakingNewCanvas = true;
    on_get_canvas_settings(200, 200, "New");
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
    return m_dlg_brushSettings->getBrushSize();
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

QFont MainWindow::getTextFont()
{
    return m_dlg_textSettings->getFont();
}

//todo. pass mousewheel events to canvas for zoom. because clicking on other dialogs means you cant zoom until clicked back on mainwindow
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

    if(m_dlg_tools->getCurrentTool() == TOOL_TEXT)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->writeText(event->text(), m_dlg_textSettings->getFont());
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(m_pressedKeys.find(Qt::Key_Delete) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->deleteKeyPressed();
        }
    }

    if(m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end())
    {
        if(m_pressedKeys.find(Qt::Key_C) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->copyKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_V) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->pasteKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_X) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->cutKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_S) != m_pressedKeys.end())
        {
            on_save();
        }
    }

    m_pressedKeys.remove(event->key());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    ui->c_tabWidget->resize(geometry().width(), geometry().height() - 40);//todo make the 40 a constant or caculate it
}

void MainWindow::moveEvent(QMoveEvent *moveEvent)
{
    QMainWindow::moveEvent(moveEvent);

    if(m_dlg_tools)
        m_dlg_tools->move(pos().x(), (geometry().height() / 2) - (m_dlg_tools->geometry().height() / 2));

    if(m_dlg_colorPicker)
        m_dlg_colorPicker->move(geometry().right() - m_dlg_colorPicker->geometry().width(), (geometry().bottom() / 2) - (m_dlg_colorPicker->geometry().height() / 2));

    if(m_dlg_brushSettings)
        m_dlg_brushSettings->move((geometry().right() - geometry().left())/2 + geometry().left(), geometry().top());
}

void MainWindow::on_get_canvas_settings(int width, int height, QString name)
{
    if(m_bMakingNewCanvas)
    {
        QImage newImage = QImage(QSize(width, height), QImage::Format_ARGB32);
        QPainter painter(&newImage);
        painter.setCompositionMode (QPainter::CompositionMode_Clear);
        painter.fillRect(QRect(0, 0, newImage.width(), newImage.height()), Qt::transparent);

        loadNewCanvas(newImage, name);
    }
    else
    {
        ui->c_tabWidget->setTabText(ui->c_tabWidget->currentIndex(), name);
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->updateSettings(width, height, name);
        }
    }
}

void MainWindow::updatedCurrentTool(Tool tool)
{
    if(tool == TOOL_TEXT)
    {
        m_dlg_textSettings->show();
    }
    else
    {
        m_dlg_textSettings->hide();
    }

    if(tool == TOOL_PAINT || tool == TOOL_ERASER)
    {
        m_dlg_brushSettings->show();
    }
    else
    {
        m_dlg_brushSettings->hide();
    }
}

void MainWindow::on_update_font(QFont font)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->updateText(font);
    }
}

void MainWindow::on_btn_addTab_clicked()
{
    m_bMakingNewCanvas = true;
    m_dlg_canvasSettings->show();
}

void MainWindow::on_btn_canvasSettings_clicked()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        m_dlg_canvasSettings->setCurrentValues(c->width(), c->height(), ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()));

        m_bMakingNewCanvas = false;
        m_dlg_canvasSettings->show();
    }
}

void MainWindow::loadNewCanvas(QImage image, QString name, QString savePath)
{
    Canvas* c = new Canvas(this, image);

    c->setSavePath(savePath);

    c->updateCurrentTool(m_dlg_tools->getCurrentTool());    
    connect(m_dlg_tools, SIGNAL(currentToolUpdated(Tool)), c, SLOT(updateCurrentTool(Tool)));

    ui->c_tabWidget->addTab(c, name);
}

void MainWindow::on_load()
{
    QFileDialog loadDialog;
    const QUrl fileUrl = loadDialog.getOpenFileUrl(this);
    QString filePath = fileUrl.path();
    QString fileName = QFileInfo(fileUrl.fileName()).baseName();

    //todo ~ why do i need to cut the first / of the url?
    filePath = filePath.mid(1, filePath.length());

    QImage image(filePath);
    loadNewCanvas(image, fileName, filePath);
}

void MainWindow::on_save()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        QString savePath = c->getSavePath();
        if(savePath == "")
        {
            savePath = getSaveAsPath(ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()));
        }

        saveCanvas(c, savePath);
    }
}

void MainWindow::on_save_as()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        QImage image = c->getImageCopy();

        const QString savePath = getSaveAsPath(ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()));

        saveCanvas(c, savePath);
    }
}

QString MainWindow::getSaveAsPath(QString name)
{
    return QFileDialog::getSaveFileName(nullptr, "file", ".", "PNG (*.png);; JPG (*.jpg);; XPM (*.xpm);; BMP (*.bmp)" );;
}

void MainWindow::saveCanvas(Canvas *canvas, QString path)
{
    canvas->setSavePath(path);

    qDebug() << path;
    qDebug() << (canvas->getImageCopy().save(path) ? "Saved image" : "Failed to save image");
}

void MainWindow::on_open_color_picker()
{
    m_dlg_colorPicker->show();
}

void MainWindow::on_open_tools()
{
    m_dlg_tools->show();
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

void MainWindow::on_c_tabWidget_tabCloseRequested(int index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->widget(index));
    ui->c_tabWidget->removeTab(index);
    delete c;
}
