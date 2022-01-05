#ifndef DLG_TOOLS_H
#define DLG_TOOLS_H

#include <QDialog>

namespace Ui {
class dlg_tools;
}

class dlg_tools : public QDialog
{
    Q_OBJECT

public:
    explicit dlg_tools(QWidget *parent = nullptr);
    ~dlg_tools();

private:
    Ui::dlg_tools *ui;
};

#endif // DLG_TOOLS_H
