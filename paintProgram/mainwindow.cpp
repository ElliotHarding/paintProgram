#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "canvas.h"

#include <QDesktopWidget>
#include <QKeyEvent>
#include <QDebug>
#include <QColor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->c_tabWidget->clear();



    //Create dialogs

    m_dlg_colorPicker = new QColorDialog(this);
    m_dlg_colorPicker->setOptions(QColorDialog::ColorDialogOption::ShowAlphaChannel | QColorDialog::ColorDialogOption::NoButtons | QColorDialog::ColorDialogOption::DontUseNativeDialog);
    m_dlg_colorPicker->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    m_dlg_colorPicker->show();

    m_dlg_canvasSettings = new DLG_SetCanvasSettings(this);

    m_dlg_tools = new DLG_Tools(this);
    m_dlg_tools->show();

    m_dlg_textSettings = new DLG_TextSettings(this);

    m_dlg_brushSettings = new DLG_BrushSettings(this);
    m_dlg_brushSettings->show();

    m_dlg_sensitivity = new DLG_Sensitivity(this);

    m_dlg_shapes = new DLG_Shapes(this);

    m_dlg_info = new DLG_Info(this);
    m_dlg_info->show();

    m_dlg_effectsSliders = new DLG_EffectsSliders(this);

    m_dlg_fileDlg = new QFileDialog(this);

    //Dialog connections
    connect(m_dlg_tools, SIGNAL(currentToolUpdated(const Tool)), this, SLOT(onCurrentToolUpdated(const Tool)));
    connect(m_dlg_textSettings, SIGNAL(updateFont(const QFont)), this, SLOT(onUpdateFont(const QFont)));
    connect(m_dlg_colorPicker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(onColorChanged(const QColor&)));
    connect(m_dlg_canvasSettings, SIGNAL(confirmCanvasSettings(int,int,QString)), this, SLOT(onGetCanvasSettings(int,int,QString)));
    connect(m_dlg_effectsSliders, SIGNAL(onBrightness(const int)), this, SLOT(onBrightness(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onContrast(const int)), this, SLOT(onContrast(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onRedLimit(const int)), this, SLOT(onRedLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onGreenLimit(const int)), this, SLOT(onGreenLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onBlueLimit(const int)), this, SLOT(onBlueLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(confirmEffects()), this, SLOT(onConfirmEffects()));
    connect(m_dlg_effectsSliders, SIGNAL(cancelEffects()), this, SLOT(onCancelEffects()));

    //Finished creating dialogs
    m_bDialogsCreated = true;


    //UI action connections
    connect(ui->actionColor_Picker, SIGNAL(triggered()), this, SLOT(onOpenColorPicker()));
    connect(ui->actionTools, SIGNAL(triggered()), this, SLOT(onOpenTools()));
    connect(ui->actionLoad_Image, SIGNAL(triggered()), this, SLOT(onLoad()));
    connect(ui->actionSave_Image, SIGNAL(triggered()), this, SLOT(onSave()));
    connect(ui->actionSave_As, SIGNAL(triggered()), this, SLOT(onSaveAs()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(onAddTabClicked()));
    connect(ui->actionImageSettings, SIGNAL(triggered()), this, SLOT(onShowCanvasSettings()));
    connect(ui->actionBlack_and_white, SIGNAL(triggered()), this, SLOT(onBlackAndWhite()));
    connect(ui->actionInvert, SIGNAL(triggered()), this, SLOT(onInvert()));
    connect(ui->actionEffectsSliders, SIGNAL(triggered()), this, SLOT(onEffectsSliders()));
    connect(ui->actionInk_Sketch, SIGNAL(triggered()), this, SLOT(onInkSketch()));
    connect(ui->actionColor_Outline, SIGNAL(triggered()), this, SLOT(onColorOutline()));

    showMaximized();

    m_bMakingNewCanvas = true;
    onGetCanvasSettings(200, 200, "New");
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
    return m_dlg_sensitivity->getSensitivity();
}

void MainWindow::setCopyBuffer(Clipboard clipboard)
{
    m_copyBuffer = clipboard;
}

Clipboard MainWindow::getCopyBuffer()
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
        if(!m_dlg_textSettings->spinBoxHasFocus())
        {
            keyPressEvent(dynamic_cast<QKeyEvent*>(event));
        }
    }
    else if(event->type() == QEvent::KeyRelease)
    {
        if(!m_dlg_textSettings->spinBoxHasFocus())
        {
            keyReleaseEvent(dynamic_cast<QKeyEvent*>(event));
        }
    }
    if(event->type() == QEvent::MouseMove)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onParentMouseMove(dynamic_cast<QMouseEvent*>(event));
        }
    }
    return QObject::eventFilter( watched, event );
}

bool MainWindow::isCtrlPressed()
{
    return m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end();
}

Shape MainWindow::getCurrentShape()
{
    return m_dlg_shapes->getShape();
}

bool MainWindow::getIsFillShape()
{
    return m_dlg_shapes->fillShape();
}

BrushShape MainWindow::getCurrentBrushShape()
{
    return m_dlg_brushSettings->getBrushShape();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    m_pressedKeys.insert(event->key());
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(m_pressedKeys.find(Qt::Key_Delete) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onDeleteKeyPressed();
        }
    }

    if(m_pressedKeys.find(Qt::Key_Control) != m_pressedKeys.end())
    {
        if(m_pressedKeys.find(Qt::Key_C) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onCopyKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_V) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onPasteKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_X) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onCutKeysPressed();
            }
        }

        else if(m_pressedKeys.find(Qt::Key_S) != m_pressedKeys.end())
        {
            onSave();
        }

        else if(m_pressedKeys.find(Qt::Key_Z) != m_pressedKeys.end())
        {
            on_btn_undo_clicked();
        }
    }
    else
    {
        if(m_dlg_tools->getCurrentTool() == TOOL_TEXT)
        {
            if(m_pressedKeys.find(event->key()) != m_pressedKeys.end())
            {
                Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
                if(c)
                {
                    c->onWriteText(event->text(), m_dlg_textSettings->getFont());
                }
            }
        }
    }

    m_pressedKeys.remove(event->key());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    ui->c_tabWidget->resize(geometry().width(), geometry().height() - ui->c_tabWidget->pos().y());

    repositionDialogs();
}

void MainWindow::moveEvent(QMoveEvent *moveEvent)
{
    QMainWindow::moveEvent(moveEvent);

    repositionDialogs();
}

void MainWindow::repositionDialogs() //todo ~ do this based of percentages that save, so when move it move to where user positioned it
{
    if(m_bDialogsCreated)
    {
        m_dlg_tools->move(pos().x(), pos().y() + (geometry().height() / 2) - (m_dlg_tools->geometry().height() / 2));

        m_dlg_colorPicker->move(geometry().right() - m_dlg_colorPicker->geometry().width(), pos().y() + (geometry().height() / 2) - (m_dlg_colorPicker->geometry().height() / 2));

        m_dlg_brushSettings->move((geometry().right() - geometry().left())/2 + geometry().left(), geometry().top());

        m_dlg_sensitivity->move((geometry().right() - geometry().left())/2 + geometry().left(), geometry().top());

        m_dlg_shapes->move(m_dlg_brushSettings->geometry().right(), geometry().top());

        m_dlg_textSettings->move((geometry().right() - geometry().left())/2 + geometry().left(), geometry().top());

        m_dlg_info->move(geometry().left(), geometry().bottom() - m_dlg_info->height() - (m_dlg_info->height()/2));
    }
}

void MainWindow::onGetCanvasSettings(int width, int height, QString name)
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
            c->onUpdateSettings(width, height, name);
        }
    }
}

void MainWindow::onCurrentToolUpdated(Tool tool)
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

    if(tool == TOOL_BUCKET || tool == TOOL_SPREAD_ON_SIMILAR)
    {
        m_dlg_sensitivity->show();
    }
    else
    {
        m_dlg_sensitivity->hide();
    }

    if(tool == TOOL_SHAPE)
    {
        m_dlg_shapes->show();
        m_dlg_brushSettings->show();
    }
    else
    {
        m_dlg_shapes->hide();
    }

    for(int i = 0; i < ui->c_tabWidget->count(); i++)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->widget(i));
        if(c)
        {
            c->onCurrentToolUpdated(tool);
        }
    }
}

void MainWindow::onUpdateFont(const QFont font)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onUpdateText(font);
    }
}

void MainWindow::onAddTabClicked()
{
    m_bMakingNewCanvas = true;
    m_dlg_canvasSettings->show();
}

void MainWindow::onShowCanvasSettings()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        m_dlg_canvasSettings->setCurrentValues(c->width(), c->height(), ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()));

        m_bMakingNewCanvas = false;
        m_dlg_canvasSettings->show();
    }
}

void MainWindow::onColorChanged(const QColor &color)
{
    if(m_dlg_tools->getCurrentTool() == TOOL_TEXT)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onUpdateText(m_dlg_textSettings->getFont());
        }
    }
}

void MainWindow::onBlackAndWhite()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onBlackAndWhite();
    }
}

void MainWindow::onInvert()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onInvert();
    }
}

void MainWindow::onEffectsSliders()
{
    m_dlg_effectsSliders->show();
}

void MainWindow::onBrightness(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onBrightness(value);
    }
}

void MainWindow::onContrast(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onContrast(value);
    }
}

void MainWindow::onRedLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onRedLimit(value);
    }
}

void MainWindow::onBlueLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onBlueLimit(value);
    }
}

void MainWindow::onGreenLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onGreenLimit(value);
    }
}

void MainWindow::onInkSketch()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onInkSketch();
    }
}

void MainWindow::onColorOutline()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onColorOutline();
    }
}

void MainWindow::onConfirmEffects()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onConfirmEffects();
    }
}

void MainWindow::onCancelEffects()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onCancelEffects();
    }
}

void MainWindow::loadNewCanvas(QImage image, QString name, QString savePath)
{
    Canvas* c = new Canvas(this, image);

    c->setSavePath(savePath);

    c->onCurrentToolUpdated(m_dlg_tools->getCurrentTool());
    connect(c, SIGNAL(selectionAreaResize(const int, const int)), m_dlg_info, SLOT(onSelectionAreaResize(const int, const int)));
    connect(c, SIGNAL(mousePositionChange(const int, const int)), m_dlg_info, SLOT(onMousePositionChange(const int, const int)));
    connect(c, SIGNAL(canvasSizeChange(const int, const int)), m_dlg_info, SLOT(onCanvasSizeChange(const int, const int)));

    const int index = ui->c_tabWidget->addTab(c, name);
    c->onAddedToTab();
    ui->c_tabWidget->setCurrentIndex(index);
}

void MainWindow::onLoad()
{
    const QUrl fileUrl = m_dlg_fileDlg->getOpenFileUrl(this);
    QString filePath = fileUrl.path();
    QString fileName = QFileInfo(fileUrl.fileName()).baseName();

    //todo ~ why do i need to cut the first / of the url?
    filePath = filePath.mid(1, filePath.length());

    QImage image(filePath);
    loadNewCanvas(image, fileName, filePath);
}

void MainWindow::onSave()
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

void MainWindow::onSaveAs()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        const QString savePath = getSaveAsPath(ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()));

        saveCanvas(c, savePath);
    }
}

QString MainWindow::getSaveAsPath(QString name)
{
    return m_dlg_fileDlg->getSaveFileName(this, name, ".", "PNG (*.png);; JPG (*.jpg);; XPM (*.xpm);; BMP (*.bmp)" );
}

void MainWindow::saveCanvas(Canvas *canvas, QString path)
{
    canvas->setSavePath(path);

    qDebug() << path;
    qDebug() << (canvas->getImageCopy().save(path) ? "Saved image" : "Failed to save image");
}

void MainWindow::onOpenColorPicker()
{
    m_dlg_colorPicker->show();
}

void MainWindow::onOpenTools()
{
    m_dlg_tools->show();
}

void MainWindow::on_btn_undo_clicked()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onUndoPressed();
    }
}

void MainWindow::on_btn_redo_clicked()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onRedoPressed();
    }
}

void MainWindow::on_c_tabWidget_tabCloseRequested(int index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->widget(index));
    ui->c_tabWidget->removeTab(index);
    delete c;
}
