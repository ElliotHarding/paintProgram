#ifndef DLG_INFO_H
#define DLG_INFO_H

#include <QDialog>

namespace Ui {
class DLG_Info;
}

//Selection area size & Mouse position & canvas size

class DLG_Info : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Info(QWidget *parent = nullptr);
    ~DLG_Info();

public slots:
    void onSelectionAreaResize(const int x, const int y);
    void onMousePositionChange(const int x, const int y);
    void onCanvasSizeChange(const int x, const int y);

private:
    Ui::DLG_Info *ui;
};

#endif // DLG_INFO_H
