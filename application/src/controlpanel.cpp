#include "controlpanel.h"
#include "ui_controlpanel.h"

ControlPanel::ControlPanel(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ControlPanel)
{
    ui->setupUi(this);
}

ControlPanel::~ControlPanel()
{
    delete ui;
}

void ControlPanel::on_actionStart_Reflow_triggered()
{
    ui->actionStart_Reflow->setEnabled(false);
    ui->actionStop_Reflow->setEnabled(true);
}

void ControlPanel::on_actionStop_Reflow_triggered()
{
    ui->actionStart_Reflow->setEnabled(true);
    ui->actionStop_Reflow->setEnabled(false);
}
