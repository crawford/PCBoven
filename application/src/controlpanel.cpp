#include "controlpanel.h"
#include "ui_controlpanel.h"

ControlPanel::ControlPanel(QWidget *parent) : QMainWindow(parent), ui(new Ui::ControlPanel)
{
	_reflowing = false;
	_ovenManager = new OvenManager(this);
	connect(_ovenManager, &OvenManager::settingsSent, this, &ControlPanel::enableActions);
	connect(_ovenManager, &OvenManager::connected, this, &ControlPanel::ovenConnected);
	connect(_ovenManager, &OvenManager::disconnected, this, &ControlPanel::ovenDisconnected);

	ui->setupUi(this);
	lblConnectionStatus = new QLabel("Not Connected");
	ui->statusBar->addPermanentWidget(lblConnectionStatus);

	_ovenManager->start();
}

ControlPanel::~ControlPanel()
{
	delete ui;
}

void ControlPanel::on_actionStart_Reflow_triggered()
{
	_reflowing = true;
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(false);
	_ovenManager->setFilamentsEnabled(true);
}

void ControlPanel::on_actionStop_Reflow_triggered()
{
	_reflowing = false;
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(false);
	_ovenManager->setFilamentsEnabled(false);
}

void ControlPanel::ovenConnected()
{
	_reflowing = false;
	enableActions();
	lblConnectionStatus->setText("Connected");
}

void ControlPanel::ovenDisconnected()
{
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(false);
	lblConnectionStatus->setText("Not Connected");
}

void ControlPanel::enableActions()
{
	ui->actionStart_Reflow->setEnabled(!_reflowing);
	ui->actionStop_Reflow->setEnabled(_reflowing);
}
