#ifndef DLG_TOOLS_H
#define DLG_TOOLS_H

#include <QDialog>

#include "tools.h"

namespace Ui {
class dlg_tools;
}

class DLG_Tools : public QDialog
{
    Q_OBJECT

public:
    explicit DLG_Tools(QWidget *parent = nullptr);
    ~DLG_Tools();

    Tool getCurrentTool() { return m_currentTool; }

signals:
    void currentToolUpdated(const Tool tool);

private slots:
    void on_btn_selectTool_clicked();
    void on_btn_paintTool_clicked();
    void on_btn_selectSpreadTool_clicked();
    void on_btn_eraserTool_clicked();
    void on_btn_dragTool_clicked();
    void on_btn_bucketTool_clicked();
    void on_btn_colorPickerTool_clicked();
    void on_btn_panTool_clicked();
    void on_btn_textTool_clicked();
    void on_btn_shapeTool_clicked();

private:
    Ui::dlg_tools *ui;

    void setCurrentTool(const Tool tool);
    Tool m_currentTool = TOOL_PAINT;
};

#endif // DLG_TOOLS_H
