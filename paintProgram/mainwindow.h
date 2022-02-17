#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//todo ~ colorPicker ~ shapes ~ text
// - pass zoom down from parent
// - when selecting color for text update immediatly...
// - typing font size causes text to be written

#include <QMainWindow>
#include <QColorDialog>
#include <QFileDialog>
#include <QSet>

#include "dlg_setcanvassettings.h"
#include "dlg_tools.h"
#include "dlg_textsettings.h"
#include "dlg_brushsettings.h"
#include "dlg_sensitivity.h"
#include "dlg_shapes.h"
#include "dlg_info.h"
#include "dlg_effectssliders.h"
#include "dlg_sketch.h"
#include "dlg_layers.h"

#include "canvas.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    ///Color picker dlg to/from canvas
    QColor getSelectedColor();
    void setSelectedColor(QColor col);

    ///Shape dlg to/from canvas
    Shape getCurrentShape();
    bool getIsFillShape();

    ///Brush dlg to canvas
    BrushShape getCurrentBrushShape();
    int getBrushSize();

    ///Sensitivity dlg to canvas
    int getSpreadSensitivity();

    ///Test settings dlg to canvas
    QFont getTextFont();

    ///Access mainwindow properties
    bool isCtrlPressed();
    void setCopyBuffer(Clipboard clipboard);
    Clipboard getCopyBuffer();

protected: //todo - can remove the key events because event filter handles them....
    bool eventFilter(QObject* watched, QEvent* event ) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* moveEvent) override;

private slots:

    ///Tool slots
    void onOpenTools();
    void onCurrentToolUpdated(Tool tool);

    ///DLG_TextSettings communication
    void onUpdateFont(const QFont font);

    ///Canvas save/load/change settings/add
    void onLoad();
    void onSave();
    void onSaveAs();
    void onAddTabClicked();
    void onGetCanvasSettings(int width, int height, QString name);
    void onShowCanvasSettings();

    ///m_dlg_colorPicker commuincations
    void onColorChanged(const QColor& color);
    void onOpenColorPicker();

    ///Showing tool dialogs
    void onShowInfoDialog();
    void onShowLayersDialog();
    void onShowColorPickerDialog();
    void onShowToolSelectorDialog();
    void onShowToolSpecificDialogs();

    ///Slots from effects dialog
    void onBlackAndWhite();
    void onInvert();
    void onEffectsSliders();
    void onSketchAndOutline();
    void onBrightness(const int value);
    void onContrast(const int value);
    void onRedLimit(const int value);
    void onBlueLimit(const int value);
    void onGreenLimit(const int value);
    void onOutlineEffect(const int value);
    void onSketchEffect(const int value);
    void onConfirmEffects();
    void onCancelEffects();

    ///Layer stuff slots
    void onLayerAdded(const uint id);
    void onLayerDeleted(const uint id);
    void onLayerEnabledChanged(const uint id, const bool enabled);

    ///Mainwindow control slots
    void on_btn_undo_clicked();
    void on_btn_redo_clicked();
    void on_c_tabWidget_tabCloseRequested(int index);

private:
    Ui::MainWindow *ui;

    //Dialogs
    DLG_SetCanvasSettings* m_dlg_canvasSettings = nullptr;
    DLG_Tools* m_dlg_tools = nullptr;
    DLG_TextSettings* m_dlg_textSettings = nullptr;
    DLG_BrushSettings* m_dlg_brushSettings = nullptr;
    DLG_Sensitivity* m_dlg_sensitivity = nullptr;
    DLG_Shapes* m_dlg_shapes = nullptr;
    DLG_Info* m_dlg_info = nullptr;
    DLG_EffectsSliders* m_dlg_effectsSliders = nullptr;
    DLG_Sketch* m_dlg_sketch = nullptr;
    DLG_Layers* m_dlg_layers = nullptr;
    QColorDialog* m_dlg_colorPicker = nullptr;
    QFileDialog* m_dlg_fileDlg = nullptr;

    QSet<int> m_pressedKeys;

    Clipboard m_copyBuffer;

    ///Loading
    void loadNewCanvas(QImage image, QString name, QString savePath = "");
    bool m_bMakingNewCanvas = false;

    ///Saving
    QString getSaveAsPath(QString name);
    void saveCanvas(Canvas* canvas, QString path);

    ///Positioning
    void repositionDialogs();
    bool m_bDialogsCreated = false;
};
#endif // MAINWINDOW_H
