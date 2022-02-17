#ifndef DLG_SKETCH_H
#define DLG_SKETCH_H

#include <QDialog>

namespace Ui {
class DLG_Sketch;
}

class DLG_Sketch : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Sketch(QWidget *parent = nullptr);
    ~DLG_Sketch();

private:
    Ui::DLG_Sketch *ui;
};

#endif // DLG_SKETCH_H
