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

    m_dlg_blurSettings = new DLG_BlurSettings(this);

    m_dlg_colorMultipliers = new DLG_ColorMultipliers(this);

    m_dlg_sketch = new DLG_Sketch(this);

    m_dlg_layers = new DLG_Layers(this);
    m_dlg_layers->show();

    m_dlg_message = new DLG_Message(this);

    m_dlg_fileDlg = new QFileDialog(this);

    //Dialog connections
    connect(m_dlg_tools, SIGNAL(currentToolUpdated(const Tool)), this, SLOT(onCurrentToolUpdated(const Tool)));
    connect(m_dlg_textSettings, SIGNAL(fontUpdated()), this, SLOT(onFontUpdated()));
    connect(m_dlg_colorPicker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(onColorChanged(const QColor&)));
    connect(m_dlg_canvasSettings, SIGNAL(confirmCanvasSettings(int,int,QString)), this, SLOT(onGetCanvasSettings(int,int,QString)));
    connect(m_dlg_effectsSliders, SIGNAL(onBrightness(const int)), this, SLOT(onBrightness(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onContrast(const int)), this, SLOT(onContrast(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onRedLimit(const int)), this, SLOT(onRedLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onGreenLimit(const int)), this, SLOT(onGreenLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(onBlueLimit(const int)), this, SLOT(onBlueLimit(const int)));
    connect(m_dlg_effectsSliders, SIGNAL(confirmEffects()), this, SLOT(onConfirmEffects()));
    connect(m_dlg_effectsSliders, SIGNAL(cancelEffects()), this, SLOT(onCancelEffects()));
    connect(m_dlg_blurSettings, SIGNAL(onNormalBlur(const int, const int, const bool)), this, SLOT(onNormalBlur(const int, const int, const bool)));
    connect(m_dlg_blurSettings, SIGNAL(confirmEffects()), this, SLOT(onConfirmEffects()));
    connect(m_dlg_blurSettings, SIGNAL(cancelEffects()), this, SLOT(onCancelEffects()));
    connect(m_dlg_colorMultipliers, SIGNAL(onColorMultipliers(const int, const int, const int, const int, const int, const int, const int, const int, const int, const int)), this, SLOT(onColorMultipliers(const int, const int, const int, const int, const int, const int, const int, const int, const int, const int)));
    connect(m_dlg_colorMultipliers, SIGNAL(confirmEffects()), this, SLOT(onConfirmEffects()));
    connect(m_dlg_colorMultipliers, SIGNAL(cancelEffects()), this, SLOT(onCancelEffects()));
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
    connect(ui->actionBlur, SIGNAL(triggered()), this, SLOT(onShowBlurDialog()));
    connect(ui->actionMultipliers, SIGNAL(triggered()), this, SLOT(onShowColorMultipliersDialog()));
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

QFont MainWindow::getTextFont()
{
    return m_dlg_textSettings->getFont();
}

void MainWindow::setLayers(QList<CanvasLayerInfo> layerInfo, uint selectedLayer)
{
    m_dlg_layers->setLayers(layerInfo, selectedLayer);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        if(!m_dlg_textSettings->spinBoxHasFocus())
        {
            keyPress(dynamic_cast<QKeyEvent*>(event));
        }
    }
    else if(event->type() == QEvent::KeyRelease)
    {
        if(!m_dlg_textSettings->spinBoxHasFocus())
        {
            keyRelease(dynamic_cast<QKeyEvent*>(event));
        }
    }
    else if(event->type() == QEvent::MouseMove)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onParentMouseMove(dynamic_cast<QMouseEvent*>(event));
        }
        else
        {
            qDebug() << "MainWindow::eventFilter - cant find canvas! ";
        }
    }
    else if(event->type() == QEvent::Wheel)
    {
        if(dynamic_cast<DLG_Tools*>(watched) || dynamic_cast<DLG_TextSettings*>(watched) || dynamic_cast<DLG_BrushSettings*>(watched) || dynamic_cast<DLG_Shapes*>(watched) ||
           dynamic_cast<DLG_Info*>(watched) || dynamic_cast<DLG_Layers*>(watched) || dynamic_cast<QColorDialog*>(watched) || dynamic_cast<DLG_Sensitivity*>(watched))
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onParentMouseScroll(dynamic_cast<QWheelEvent*>(event));
            }
            else
            {
                qDebug() << "MainWindow::eventFilter - cant find canvas! ";
            }
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

void MainWindow::keyPress(QKeyEvent *event)
{
    m_pressedKeys.insert(event->key());
}

void MainWindow::keyRelease(QKeyEvent *event)
{
    if(m_pressedKeys.find(Qt::Key_Delete) != m_pressedKeys.end())
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onDeleteKeyPressed();
        }
        else
        {
            qDebug() << "MainWindow::keyReleaseEvent - cant find canvas! ";
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
            else
            {
                qDebug() << "MainWindow::keyReleaseEvent - cant find canvas! ";
            }
        }

        else if(m_pressedKeys.find(Qt::Key_V) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onPasteKeysPressed();
            }
            else
            {
                qDebug() << "MainWindow::keyReleaseEvent - cant find canvas! ";
            }
        }

        else if(m_pressedKeys.find(Qt::Key_X) != m_pressedKeys.end())
        {
            Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
            if(c)
            {
                c->onCutKeysPressed();
            }
            else
            {
                qDebug() << "MainWindow::keyReleaseEvent - cant find canvas! ";
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
                    c->onWriteText(event->text());
                }
                else
                {
                    qDebug() << "MainWindow::keyReleaseEvent - cant find canvas! ";
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

        m_dlg_blurSettings->move(geometry().center().x() - (geometry().center().x() - geometry().left())/2 - m_dlg_blurSettings->geometry().width()/2, geometry().top());

        m_dlg_effectsSliders->move(geometry().center().x() - (geometry().center().x() - geometry().left())/2 - m_dlg_effectsSliders->geometry().width()/2, geometry().top());

        m_dlg_sketch->move(geometry().center().x() - (geometry().center().x() - geometry().left())/2 - m_dlg_sketch->geometry().width()/2, geometry().top());

        m_dlg_colorMultipliers->move(geometry().center().x() - (geometry().center().x() - geometry().left())/2 - m_dlg_colorMultipliers->geometry().width()/2, geometry().top());
    }
}

void MainWindow::onGetCanvasSettings(int width, int height, QString name)
{
    if(m_bMakingNewCanvas)
    {
        Canvas* c = new Canvas(this, width, height);
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
        else
        {
            qDebug() << "MainWindow::onGetCanvasSettings - cant find canvas! ";
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
        else
        {
            qDebug() << "MainWindow::onCurrentToolUpdated - cant find canvas! " << i;
        }
    }
}

void MainWindow::onFontUpdated()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onUpdateText();
    }
    else
    {
        qDebug() << "MainWindow::onFontUpdated - cant find canvas!";
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
    else
    {
        qDebug() << "MainWindow::onShowCanvasSettings - cant find canvas!";
    }
}

void MainWindow::onColorChanged(const QColor &color)
{
    if(m_dlg_tools->getCurrentTool() == TOOL_TEXT)
    {
        Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
        if(c)
        {
            c->onUpdateText();
        }
        else
        {
            qDebug() << "MainWindow::onColorChanged - cant find canvas!";
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
    else
    {
        qDebug() << "MainWindow::onBlackAndWhite - cant find canvas!";
    }
}

void MainWindow::onInvert()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onInvert();
    }
    else
    {
        qDebug() << "MainWindow::onInvert - cant find canvas!";
    }
}

void MainWindow::onEffectsSliders()
{
    m_dlg_effectsSliders->show();
}

void MainWindow::onShowBlurDialog()
{
    m_dlg_blurSettings->show();
}

void MainWindow::onShowColorMultipliersDialog()
{
    m_dlg_colorMultipliers->show();
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
    else
    {
        qDebug() << "MainWindow::onBrightness - cant find canvas!";
    }
}

void MainWindow::onContrast(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onContrast(value);
    }
    else
    {
        qDebug() << "MainWindow::onContrast - cant find canvas!";
    }
}

void MainWindow::onRedLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onRedLimit(value);
    }
    else
    {
        qDebug() << "MainWindow::onRedLimit - cant find canvas!";
    }
}

void MainWindow::onBlueLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onBlueLimit(value);
    }
    else
    {
        qDebug() << "MainWindow::onBlueLimit - cant find canvas!";
    }
}

void MainWindow::onGreenLimit(int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onGreenLimit(value);
    }
    else
    {
        qDebug() << "MainWindow::onGreenLimit - cant find canvas!";
    }
}

void MainWindow::onOutlineEffect(const int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onOutlineEffect(value);
    }
    else
    {
        qDebug() << "MainWindow::onOutlineEffect - cant find canvas!";
    }
}

void MainWindow::onSketchEffect(const int value)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onSketchEffect(value);
    }
    else
    {
        qDebug() << "MainWindow::onSketchEffect - cant find canvas!";
    }
}

void MainWindow::onNormalBlur(const int difference, const int averageArea, const bool includeTransparent)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onNormalBlur(difference, averageArea, includeTransparent);
    }
    else
    {
        qDebug() << "MainWindow::onNormalBlur - cant find canvas!";
    }
}

void MainWindow::onColorMultipliers(const int redXred, const int redXgreen, const int redXblue, const int greenXred, const int greenXgreen, const int greenXblue, const int blueXred, const int blueXgreen, const int blueXblue, const int xTransparent)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onColorMultipliers(redXred, redXgreen, redXblue, greenXred, greenXgreen, greenXblue, blueXred, blueXgreen, blueXblue, xTransparent);
    }
    else
    {
        qDebug() << "MainWindow::onColorMultipliers - cant find canvas!";
    }
}

void MainWindow::onConfirmEffects()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onConfirmEffects();
    }
    else
    {
        qDebug() << "MainWindow::onConfirmEffects - cant find canvas!";
    }
}

void MainWindow::onCancelEffects()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onCancelEffects();
    }
    else
    {
        qDebug() << "MainWindow::onCancelEffects - cant find canvas!";
    }
}

void MainWindow::onLayerAdded()
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerAdded();
    }
    else
    {
        qDebug() << "MainWindow::onLayerAdded - cant find canvas!";
    }
}

void MainWindow::onLayerDeleted(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerDeleted(index);
    }
    else
    {
        qDebug() << "MainWindow::onLayerDeleted - cant find canvas!";
    }
}

void MainWindow::onLayerEnabledChanged(const uint index, const bool enabled)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerEnabledChanged(index, enabled);
    }
    else
    {
        qDebug() << "MainWindow::onLayerEnabledChanged - cant find canvas!";
    }
}

void MainWindow::onLayerTextChanged(const uint index, QString text)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerTextChanged(index, text);
    }
    else
    {
        qDebug() << "MainWindow::onLayerTextChanged - cant find canvas!";
    }
}

void MainWindow::onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMergeRequested(layerIndexA, layerIndexB);
    }
    else
    {
        qDebug() << "MainWindow::onLayerMergeRequested - cant find canvas!";
    }
}

void MainWindow::onLayerMoveUp(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMoveUp(index);
    }
    else
    {
        qDebug() << "MainWindow::onLayerMoveUp - cant find canvas!";
    }
}

void MainWindow::onLayerMoveDown(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onLayerMoveDown(index);
    }
    else
    {
        qDebug() << "MainWindow::onLayerMoveDown - cant find canvas!";
    }
}

void MainWindow::onSelectedLayerChanged(const uint index)
{
    Canvas* c = dynamic_cast<Canvas*>(ui->c_tabWidget->currentWidget());
    if(c)
    {
        c->onSelectedLayerChanged(index);
    }
    else
    {
        qDebug() << "MainWindow::onSelectedLayerChanged - cant find canvas!";
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

    if(filePath == "")
    {
        return;
    }

    bool loadSuccess = false;
    Canvas* c = new Canvas(this, filePath, loadSuccess);
    if(loadSuccess)
    {
        addNewCanvas(c, fileName);
    }
    else
    {
        delete c;
        m_dlg_message->show("Canvas load has failed!");
    }
}

void MainWindow::onLoadLayer()
{
    if(ui->c_tabWidget->count() == 0)
    {
        m_dlg_message->show("No canvas to load layer into!");
        return;
    }

    //Get file path and name
    const QUrl fileUrl = m_dlg_fileDlg->getOpenFileUrl(this);
    QString filePath = fileUrl.path();
    QString fileName = QFileInfo(fileUrl.fileName()).baseName();
    filePath = filePath.mid(1, filePath.length());//todo ~ why do i need to cut the first / of the url?

    if(filePath == "")
    {
        return;
    }

    //Create & validate image
    QImage image(filePath);
    if(image == QImage() || image.isNull())
    {
        m_dlg_message->show("Failed to load layer! \n Ensure file is of correct type.");
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
    else
    {
        m_dlg_message->show("No canvas to save!");
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
    else
    {
        m_dlg_message->show("No canvas to save!");
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
    else
    {
        m_dlg_message->show("No canvas to export image!");
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
