#ifndef DLG_BRUSHSETTINGS_H
#define DLG_BRUSHSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_BrushSettings;
}

enum BrushShape
{
    BRUSHSHAPE_RECT,
    BRUSHSHAPE_CIRCLE
};

class DLG_BrushSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_BrushSettings(QWidget *parent = nullptr);
    ~DLG_BrushSettings();

    int getBrushSize();
    BrushShape getBrushShape();

private slots:
    void on_btn_shapeRect_clicked();
    void on_btn_shapeCircle_clicked();

private:
    Ui::DLG_BrushSettings *ui;

    BrushShape m_brushShape;
};

#endif // DLG_BRUSHSETTINGS_H
