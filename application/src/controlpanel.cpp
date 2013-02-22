#include <QMessageBox>
#include <errno.h>
#include "controlpanel.h"
#include "ui_controlpanel.h"

ControlPanel::ControlPanel(QWidget *parent) : QMainWindow(parent), ui(new Ui::ControlPanel)
{
	_reflowing = false;
	_ovenManager = new OvenManager(this);
	connect(_ovenManager, &OvenManager::errorOccurred, this, &ControlPanel::handleError);
	connect(_ovenManager, &OvenManager::connected, this, &ControlPanel::ovenConnected);
	connect(_ovenManager, &OvenManager::disconnected, this, &ControlPanel::ovenDisconnected);

	ui->setupUi(this);
	connectionStatus = new QLabel("Waiting for connection");
	ui->statusBar->addPermanentWidget(connectionStatus);

	QMap<QTime, int> vtemps;
	vtemps.insert(QTime(0, 0, 0), 0);
	vtemps.insert(QTime(0, 0, 10), 150);
	vtemps.insert(QTime(0, 0, 15), 200);
	vtemps.insert(QTime(0, 0, 30), 0);
	ui->reflowGraph->setTemperatureTargets(vtemps);

	_ovenManager->start();
}

ControlPanel::~ControlPanel()
{
	delete ui;
}

void ControlPanel::on_actionStart_Reflow_triggered()
{
	_reflowing = true;
	ui->reflowGraph->clearGraph();
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(true);
	_ovenManager->setFilamentsEnabled(true);

	QPair<QTime, int> temps[] = { QPair<QTime, int>(QTime(0, 0, 0), 5),
								  QPair<QTime, int>(QTime(0, 0, 2), 10),
								  QPair<QTime, int>(QTime(0, 0, 3), 20),
								  QPair<QTime, int>(QTime(0, 0, 5), 25),
								  QPair<QTime, int>(QTime(0, 0, 7), 28),
								  QPair<QTime, int>(QTime(0, 0, 10), 30),
								  QPair<QTime, int>(QTime(0, 0, 15), 31),
								  QPair<QTime, int>(QTime(0, 0, 20), 23),
								  QPair<QTime, int>(QTime(0, 0, 25), 26),
								  QPair<QTime, int>(QTime(0, 0, 30), 17)};
	for (unsigned int i = 0; i < sizeof(temps)/sizeof(*temps); i++)
		ui->reflowGraph->addTemperature(temps[i].first, temps[i].second);
}

void ControlPanel::on_actionStop_Reflow_triggered()
{
	_reflowing = false;
	ui->actionStart_Reflow->setEnabled(true);
	ui->actionStop_Reflow->setEnabled(false);
	_ovenManager->setFilamentsEnabled(false);
}

void ControlPanel::ovenConnected()
{
	_reflowing = false;
	ui->actionStart_Reflow->setEnabled(true);
	ui->actionStop_Reflow->setEnabled(false);
	connectionStatus->setText("Connected");
}

void ControlPanel::ovenDisconnected()
{
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(false);
	connectionStatus->setText("Disconnected");
}

void ControlPanel::handleError(int error)
{
	ui->statusBar->showMessage(QString("An error occured (%1)").arg(error));
	switch (error) {
	case ENOENT:
		QMessageBox::critical(this, "Failed to connect to oven", "An oven pluggin event was detected, but the device file could not be opened. Make sure the udev rule is installed.");
		break;
	default:
		QMessageBox::critical(this, "Well fuck me", QString().setNum(error));
		break;
	}
}

