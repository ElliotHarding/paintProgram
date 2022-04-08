#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include "dlg_message.h"
#include "dlg_blursettings.h"
#include "dlg_colormultipliers.h"
#include "dlg_huesaturation.h"
#include "dlg_borderedit.h"

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

    ///Canvas to dlg layers
    void setLayers(QList<CanvasLayerInfo> layerInfo, uint selectedLayer);

    ///Access mainwindow properties
    bool isCtrlPressed();

protected: //todo - can remove the key events because event filter handles them....
    bool eventFilter(QObject* watched, QEvent* event ) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* moveEvent) override;

private slots:
    void keyPress(QKeyEvent *event);
    void keyRelease(QKeyEvent *event);

    ///Tool slots
    void onOpenTools();
    void onCurrentToolUpdated(Tool tool);

    ///DLG_TextSettings communication
    void onFontUpdated();

    ///Canvas save/load/change settings/add
    void onLoad();
    void onLoadLayer();
    void onSave();
    void onSaveAs();
    void onExportImage();
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

    ///Slots from effects dialogs
    void onBlackAndWhite();
    void onInvert();
    void onEffectsSliders();
    void onShowBlurDialog();
    void onShowColorMultipliersDialog();
    void onShowHueSaturationDialog();
    void onShowBorderEditDialog();
    void onSketchAndOutline();
    void onBrightness(const int value);
    void onContrast(const int value);
    void onOutlineEffect(const int value);
    void onSketchEffect(const int value);
    void onNormalBlur(const int difference, const int averageArea, const bool includeTransparent);
    void onColorMultipliers(const int redXred, const int redXgreen, const int redXblue,
                            const int greenXred, const int greenXgreen, const int greenXblue,
                            const int blueXred, const int blueXgreen, const int blueXblue,
                            const int xTransparent);
    void onHueSaturation(const int hue, const int saturation);
    void onBorderEdit(const int borderEdges, const bool includeCorners, const bool removeCenter);
    void onConfirmEffects();
    void onCancelEffects();

    ///Layer stuff slots
    void onLayerAdded();
    void onLayerDeleted(const uint index);
    void onLayerEnabledChanged(const uint index, const bool enabled);
    void onLayerTextChanged(const uint index, QString text);
    void onLayerMergeRequested(const uint layerIndexA, const uint layerIndexB);
    void onLayerMoveUp(const uint index);
    void onLayerMoveDown(const uint index);
    void onSelectedLayerChanged(const uint index);

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
    DLG_ColorMultipliers* m_dlg_colorMultipliers = nullptr;
    DLG_HueSaturation* m_dlg_hueSaturation = nullptr;
    DLG_BorderEdit* m_dlg_borderEdit = nullptr;
    DLG_BlurSettings* m_dlg_blurSettings = nullptr;
    DLG_Sketch* m_dlg_sketch = nullptr;
    DLG_Layers* m_dlg_layers = nullptr;
    DLG_Message* m_dlg_message = nullptr;
    QColorDialog* m_dlg_colorPicker = nullptr;
    QFileDialog* m_dlg_fileDlg = nullptr;

    QSet<int> m_pressedKeys;

    ///Loading
    void addNewCanvas(Canvas* c, QString name);
    bool m_bMakingNewCanvas = false;

    ///Saving
    QString getSaveAsPath(QString name);
    void saveCanvas(Canvas* canvas, QString path);

    ///Positioning
    void repositionDialogs();
    bool m_bDialogsCreated = false;
};
#endif // MAINWINDOW_H
