#ifndef DLG_BLURSETTINGS_H
#define DLG_BLURSETTINGS_H

#include <QDialog>

namespace Ui {
class DLG_BlurSettings;
}

class DLG_BlurSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_BlurSettings(QWidget *parent = nullptr);
    ~DLG_BlurSettings();

private:
    Ui::DLG_BlurSettings *ui;
};

#endif // DLG_BLURSETTINGS_H
