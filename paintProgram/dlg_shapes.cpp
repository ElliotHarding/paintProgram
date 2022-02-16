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
    ui->btn_shapeCircle->setFlat(true);
    ui->btn_shapeRect->setFlat(false);
    ui->btn_shapeLine->setFlat(true);
    ui->btn_shapeTriangle->setFlat(true);
}

void DLG_Shapes::on_btn_shapeCircle_clicked()
{
    m_shape = SHAPE_CIRCLE;
    ui->btn_shapeCircle->setFlat(false);
    ui->btn_shapeRect->setFlat(true);
    ui->btn_shapeLine->setFlat(true);
    ui->btn_shapeTriangle->setFlat(true);
}

void DLG_Shapes::on_btn_shapeTriangle_clicked()
{
    m_shape = SHAPE_TRIANGLE;
    ui->btn_shapeCircle->setFlat(true);
    ui->btn_shapeRect->setFlat(true);
    ui->btn_shapeLine->setFlat(true);
    ui->btn_shapeTriangle->setFlat(false);
}

void DLG_Shapes::on_btn_shapeLine_clicked()
{
    m_shape = SHAPE_LINE;
    ui->btn_shapeCircle->setFlat(true);
    ui->btn_shapeRect->setFlat(false);
    ui->btn_shapeLine->setFlat(false);
    ui->btn_shapeTriangle->setFlat(true);
}
