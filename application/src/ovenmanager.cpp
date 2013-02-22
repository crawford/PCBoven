#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "ovenmanager.h"

#define PCB_OVEN_ENABLE_FILAMENTS  -1
#define PCB_OVEN_DISABLE_FILAMENTS -1
#define PCB_OVEN_SET_TEMPERATURE   -1

OvenManager::OvenManager(QObject *parent) : QObject(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;

	_ioctlCmdThread = new QThread(parent);
	_ioctlReadingThread = new QThread(parent);
	_udevMonitor = new UdevMonitor(parent);
	_ioctlWorker = new IoctlWorker();

	_ioctlWorker->moveToThread(_ioctlCmdThread);

	connect(this, &OvenManager::setTemperature, _ioctlWorker,&IoctlWorker::setTemperature);
	connect(this, &OvenManager::enableFilaments, _ioctlWorker,&IoctlWorker::enableFilaments);
	connect(_ioctlWorker, &IoctlWorker::errorOccurred, this, &OvenManager::errorOccurred);

	connect(_udevMonitor, &UdevMonitor::ovenProbed, this, &OvenManager::handleOvenProbed);
	connect(_udevMonitor, &UdevMonitor::ovenRemoved, this, &OvenManager::handleOvenProbed);
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
	if (enabled != _filamentsEnabled) {
		_filamentsEnabled = enabled;
		int request = enabled ? PCB_OVEN_ENABLE_FILAMENTS :
			PCB_OVEN_DISABLE_FILAMENTS;

		QThread::currentThread()->sleep(1);
		if (ioctl(_ioctlFd, request))
			emit errorOccurred(errno);
	}
}

void OvenManager::setTargetTemperature(int temperature)
{
	if (temperature != _targetTemperature) {
		_targetTemperature = temperature;
		if (ioctl(_ioctlFd, PCB_OVEN_SET_TEMPERATURE))
			emit errorOccurred(errno);
	}
}

void OvenManager::handleOvenProbed()
{
	_ioctlFd = open("/dev/bus/usb/001", O_RDONLY, O_NONBLOCK);
	_ioctlWorker->initialize();
	_ioctlCmdThread->start();
	_ioctlReadingThread->start();

	emit connected();
}

void OvenManager::handleOvenRemoved()
{
	_ioctlCmdThread->quit();
	_ioctlReadingThread->quit();
	close(_ioctlFd);

	emit disconnected();
}

