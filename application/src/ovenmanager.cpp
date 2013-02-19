#include "ovenmanager.h"

OvenManager::OvenManager(QObject *parent) : QObject(parent)
{
	_ioctlCmdThread = new QThread(parent);
	_ioctlReadingThread = new QThread(parent);
	_udevMonitor = new UdevMonitor(parent);
	_ioctlWorker = new IoctlWorker();

	_ioctlWorker->moveToThread(_ioctlCmdThread);

	connect(this, &OvenManager::settings, _ioctlWorker,&IoctlWorker::sendSettings);
	connect(_ioctlWorker, &IoctlWorker::settingsSent, this, &OvenManager::settingsSent);
	connect(_udevMonitor, &UdevMonitor::connected, this, &OvenManager::connected);
	connect(_udevMonitor, &UdevMonitor::disconnected, this, &OvenManager::disconnected);
}

OvenManager::~OvenManager()
{
	stop();
	_ioctlCmdThread->wait();
	_ioctlReadingThread->wait();
	_udevMonitor->wait();
}

void OvenManager::start()
{
	_ioctlCmdThread->start();
	_ioctlReadingThread->start();
	_udevMonitor->start();
}

void OvenManager::stop()
{
	_ioctlCmdThread->quit();
	_ioctlReadingThread->quit();
	_udevMonitor->terminate();
}

void OvenManager::setFilamentsEnabled(bool enabled)
{
	_filamentsEnabled = enabled;
	emit settings(_filamentsEnabled, _targetTemperature);
}

void OvenManager::setTargetTemperature(int temperature)
{
	_targetTemperature = temperature;
	emit settings(_filamentsEnabled, _targetTemperature);
}
