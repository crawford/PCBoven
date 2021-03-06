#include <QFile>
#include <QMessageBox>
#include <errno.h>
#include <iostream>
#include "controlpanel.h"
#include "ui_controlpanel.h"

ControlPanel::ControlPanel(QWidget *parent) : QMainWindow(parent), ui(new Ui::ControlPanel)
{
	_ovenManager = new OvenManager(this);
	connect(_ovenManager, &OvenManager::errorOccurred, this, &ControlPanel::handleError);
	connect(_ovenManager, &OvenManager::connected, this, &ControlPanel::ovenConnected);
	connect(_ovenManager, &OvenManager::disconnected, this, &ControlPanel::ovenDisconnected);

	_reflowTimer = new QTimer(this);
	_reflowTimer->setInterval(ControlPanel::REFLOW_CHECK_PERIOD_MS);
	connect(_reflowTimer, &QTimer::timeout, this, &ControlPanel::checkProfile);

	ui->setupUi(this);
	connectionStatus = new QLabel("Waiting for connection");
	reflowStatus = new QLabel(QTime(0, 0).toString());
	ui->statusBar->addPermanentWidget(connectionStatus);
	ui->statusBar->addPermanentWidget(reflowStatus);

	QFile rawProfile(qApp->arguments().last());
	if (rawProfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		_profile = ReflowProfile::parseFromJson(rawProfile.readAll());
		rawProfile.close();
		ui->reflowGraph->setTemperatureTargets(_profile.getProfile());
	} else {
		std::cerr << "Could not open '"
		          << qApp->arguments().last().toUtf8().data()
		          << "'"
		          << std::endl;
	}
	_profile.interpolate(REFLOW_STEP_PERIOD_MS);

	_ovenManager->start();
}

ControlPanel::~ControlPanel()
{
	delete ui;
}

void ControlPanel::on_actionStart_Reflow_triggered()
{
	_reflowStartTime.start();
	ui->reflowGraph->clearGraph();
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(true);
	_ovenManager->setFilamentsEnabled(true);


	connect(_ovenManager, &OvenManager::readingsRead, this, &ControlPanel::logReadings);

	_nextTarget = _profile.getProfile().constBegin();
	checkProfile();

	_ovenManager->setTargetTemperature(_nextTarget.value());
	ui->statusBar->showMessage(QString("Target temperature: %1C").arg(_nextTarget.value()));

	_reflowTimer->start();
}

void ControlPanel::on_actionStop_Reflow_triggered()
{
	_reflowTimer->stop();
	ui->actionStart_Reflow->setEnabled(true);
	ui->actionStop_Reflow->setEnabled(false);
	_ovenManager->setFilamentsEnabled(false);
	disconnect(_ovenManager, &OvenManager::readingsRead, this, &ControlPanel::logReadings);

	ui->statusBar->showMessage("Reflow stopped");
}

void ControlPanel::ovenConnected()
{
	ui->actionStart_Reflow->setEnabled(true);
	ui->actionStop_Reflow->setEnabled(false);
	connectionStatus->setText("Connected");
}

void ControlPanel::ovenDisconnected()
{
	on_actionStop_Reflow_triggered();
	ui->actionStart_Reflow->setEnabled(false);
	ui->actionStop_Reflow->setEnabled(false);
	connectionStatus->setText("Disconnected");
}

void ControlPanel::handleError(int error)
{
	ui->statusBar->showMessage(QString("An error occured (%1)").arg(error));
	switch (error) {
	case ENOENT:
		QMessageBox::critical(this, "Failed to connect to driver", "Could not find the pcboven device. Make sure that the driver is installed.");
		break;
	case EACCES:
		QMessageBox::critical(this, "Failed to connect to driver", "Could not connect to the pcboven device. For now, run this as root. TODO ALEX");
		break;
	default:
		QMessageBox::critical(this, "Well fuck me", QString().setNum(error));
		break;
	}
}

void ControlPanel::checkProfile()
{
	QTime adjustedTime = QTime(0, 0).addMSecs(_reflowStartTime.msecsTo(QTime::currentTime()));
	reflowStatus->setText(adjustedTime.toString());

	if (adjustedTime >= _nextTarget.key()) {
		if (_nextTarget != _profile.getProfile().constEnd())
			_nextTarget++;

		if (_nextTarget == _profile.getProfile().constEnd()) {
			on_actionStop_Reflow_triggered();
		} else {
			_ovenManager->setTargetTemperature(_nextTarget.value());
			ui->statusBar->showMessage(QString("Target temperature: %1C").arg(_nextTarget.value()));
		}
	}
}

void ControlPanel::logReadings(struct oven_state state, QTime timestamp)
{
	ui->reflowGraph->addTemperature(QTime(0, 0).addMSecs(_reflowStartTime.msecsTo(timestamp)), state.probe_temp);
}

