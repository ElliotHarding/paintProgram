#include "dlg_shapes.h"
#include "ui_dlg_shapes.h"

DLG_Shapes::DLG_Shapes(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DLG_Shapes)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

DLG_Shapes::~DLG_Shapes()
{
    delete ui;
}

Shape DLG_Shapes::getShape()
{
    return m_shape;
}

bool DLG_Shapes::fillShape()
{
    return ui->checkBox_fill->isChecked();
}

void DLG_Shapes::on_btn_shapeRect_clicked()
{
    m_shape = SHAPE_RECT;
}

void DLG_Shapes::on_btn_shapeCircle_clicked()
{
    m_shape = SHAPE_CIRCLE;
}

void DLG_Shapes::on_btn_shapeTriangle_clicked()
{
    m_shape = SHAPE_TRIANGLE;
}
