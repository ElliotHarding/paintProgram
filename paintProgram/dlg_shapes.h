#ifndef DLG_SHAPES_H
#define DLG_SHAPES_H

#include <QDialog>

namespace Ui {
class DLG_Shapes;
}

enum Shape
{
    SHAPE_RECT,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
};

class DLG_Shapes : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Shapes(QWidget *parent = nullptr);
    ~DLG_Shapes();

    Shape getShape();
    bool fillShape();

private slots:
    void on_btn_shapeRect_clicked();

    void on_btn_shapeCircle_clicked();

    void on_btn_shapeTriangle_clicked();

private:
    Ui::DLG_Shapes *ui;

    Shape m_shape = SHAPE_RECT;
};

#endif // DLG_SHAPES_H
