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

    m_dlg_sketch = new DLG_Sketch(this);

    m_dlg_layers = new DLG_Layers(this);
    m_dlg_layers->show();

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
    connect(m_dlg_sketch, SIGNAL(onOutlineEffect(const int)), this, SLOT(onOutlineEffect(const int)));
    connect(m_dlg_sketch, SIGNAL(onSketchEffect(const int)), this, SLOT(onSketchEffect(const int)));
    connect(m_dlg_sketch, SIGNAL(confirmEffects()), this, SLOT(onConfirmEffects()));
    connect(m_dlg_sketch, SIGNAL(cancelEffects()), this, SLOT(onCancelEffects()));
    connect(m_dlg_layers, SIGNAL(onLayerAdded()), this, SLOT(onLayerAdded()));
    connect(m_dlg_layers, SIGNAL(onLayerDeleted(const uint)), this, SLOT(onLayerDeleted(const uint)));
    connect(m_dlg_layers, SIGNAL(onLayerEnabledChanged(const uint, const bool)), this, SLOT(onLayerEnabledChanged(const uint, const bool)));
    connect(m_dlg_layers, SIGNAL(onSelectedLayerChanged(const uint)), this, SLOT(onSelectedLayerChanged(const uint)));
    connect(m_dlg_layers, SIGNAL(onLayerTextChanged(const uint, QString)), this, SLOT(onLayerTextChanged(const uint, QString)));
    connect(m_dlg_layers, SIGNAL(onLayerMergeRequested(const uint, const uint)), this, SLOT(onLayerMergeRequested(const uint, const uint)));
    connect(m_dlg_layers, SIGNAL(onLayerMoveUp(const uint)), this, SLOT(onLayerMoveUp(const uint)));
    connect(m_dlg_layers, SIGNAL(onLayerMoveDown(const uint)), this, SLOT(onLayerMoveDown(const uint)));

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
    connect(ui->actionSketch_Outline, SIGNAL(triggered()), this, SLOT(onSketchAndOutline()));
    connect(ui->action_showInfoDialog, SIGNAL(triggered()), this, SLOT(onShowInfoDialog()));
    connect(ui->action_showLayersDialog, SIGNAL(triggered()), this, SLOT(onShowLayersDialog()));
    connect(ui->action_showColorPicker, SIGNAL(triggered()), this, SLOT(onShowColorPickerDialog()));
    connect(ui->action_showToolSelector, SIGNAL(triggered()), this, SLOT(onShowToolSelectorDialog()));
    connect(ui->action_showToolSpecificDialogs, SIGNAL(triggered()), this, SLOT(onShowToolSpecificDialogs()));
    connect(ui->actionLoad_layer, SIGNAL(triggered()), this, SLOT(onLoadLayer()));
    connect(ui->actionExport, SIGNAL(triggered()), this, SLOT(onExportImage()));

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

void MainWindow::setLayers(QList<CanvasLayerInfo> layerInfo, uint selectedLayer)
{
    m_dlg_layers->setLayers(layerInfo, selectedLayer);
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

        const int titleBarHeight = 40;
        m_dlg_layers->move(geometry().right() - m_dlg_layers->width(), geometry().bottom() - m_dlg_layers->height() - titleBarHeight);
    }
}

void MainWindow::onGetCanvasSettings(int width, int height, QString name)
{
    if(m_bMakingNewCanvas)
    {
        QImage newImage = QImage(QSize(width, height), QImage::Format_ARGB32);
        newImage.fill(Qt::transparent);

        Canvas* c = new Canvas(this, newImage);
        addNewCanvas(c, name);
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

void MainWindow::onSketchAndOutline()
{
    m_dlg_sketch->show();
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

void MainWindow::onOutlineEffect(const int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onOutlineEffect(value);
    }
}

void MainWindow::onSketchEffect(const int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onSketchEffect(value);
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

void MainWindow::onLayerAdded()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerAdded();
    }
}

void MainWindow::onLayerDeleted(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerDeleted(index);
    }
}

void MainWindow::onLayerEnabledChanged(const uint index, const bool enabled)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerEnabledChanged(index, enabled);
    }
}

void MainWindow::onLayerTextChanged(const uint index, QString text)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerTextChanged(index, text);
    }
}

void MainWindow::onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMergeRequested(layerIndexA, layerIndexB);
    }
}

void MainWindow::onLayerMoveUp(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMoveUp(index);
    }
}

void MainWindow::onLayerMoveDown(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMoveDown(index);
    }
}

void MainWindow::onSelectedLayerChanged(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onSelectedLayerChanged(index);
    }
}

void MainWindow::addNewCanvas(Canvas *c, QString name)
{
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
    filePath = filePath.mid(1, filePath.length());//todo ~ why do i need to cut the first / of the url?

    Canvas* c = new Canvas(this, filePath);
    addNewCanvas(c, fileName);
}

void MainWindow::onLoadLayer()
{
    if(ui->c_tabWidget->count() == 0)
    {
        //todo - notify user that theres no canvas to add layer to
        return;
    }

    //Get file path and name
    const QUrl fileUrl = m_dlg_fileDlg->getOpenFileUrl(this);
    QString filePath = fileUrl.path();
    QString fileName = QFileInfo(fileUrl.fileName()).baseName();
    filePath = filePath.mid(1, filePath.length());//todo ~ why do i need to cut the first / of the url?

    //Create & validate image
    QImage image(filePath);
    if(image == QImage())
    {
        //todo - notify user of invalid load
        return;
    }

    //Add to current canvas
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        CanvasLayer cl;
        cl.m_image = image;
        cl.m_info.m_name = fileName;
        c->onLoadLayer(cl);
    }
    else
    {
        qDebug() << "MainWindow::onLoadLayer - Couldnt get canvas";
    }
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

void MainWindow::onExportImage()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        const QString exportPath = m_dlg_fileDlg->getSaveFileName(this, ui->c_tabWidget->tabText(ui->c_tabWidget->currentIndex()), ".", "PNG (*.png);; JPG (*.jpg);; XPM (*.xpm);; BMP (*.bmp)" );

        qDebug() << "MainWindow::onExportImage:";
        qDebug() << exportPath;
        qDebug() << (c->getImageCopy().save(exportPath) ? "Saved image" : "Failed to save image");
    }
}

QString MainWindow::getSaveAsPath(QString name)
{
    return m_dlg_fileDlg->getSaveFileName(this, name, ".", "PPG (*.paintProgram)" );
}

void MainWindow::saveCanvas(Canvas *canvas, QString path)
{
    qDebug() << "MainWindow::saveCanvas:";
    qDebug() << path;
    qDebug() << (canvas->save(path) ? "Saved image" : "Failed to save image");
}

void MainWindow::onOpenColorPicker()
{
    m_dlg_colorPicker->show();
}

void MainWindow::onShowInfoDialog()
{
    m_dlg_info->show();
}

void MainWindow::onShowLayersDialog()
{
    m_dlg_layers->show();
}

void MainWindow::onShowColorPickerDialog()
{
    m_dlg_colorPicker->show();
}

void MainWindow::onShowToolSelectorDialog()
{
    m_dlg_tools->show();
}

void MainWindow::onShowToolSpecificDialogs()
{
    onCurrentToolUpdated(m_dlg_tools->getCurrentTool());
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
