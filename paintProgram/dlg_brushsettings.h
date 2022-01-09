#ifndef DLG_BRUSHSETTINGS_H
#define DLG_BRUSHSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_BrushSettings;
}

class DLG_BrushSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_BrushSettings(QWidget *parent = nullptr);
    ~DLG_BrushSettings();

    int getBrushSize();

private:
    Ui::DLG_BrushSettings *ui;
};

#endif // DLG_BRUSHSETTINGS_H
