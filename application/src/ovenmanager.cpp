#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <errno.h>
#include "ovenmanager.h"
#include "pcboven_usb.h"

OvenManager::OvenManager(QObject *parent) : QObject(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;

	_udevMonitor = new UdevMonitor(parent);

	connect(_udevMonitor, &UdevMonitor::ovenProbed, this, &OvenManager::handleOvenProbed);
	connect(_udevMonitor, &UdevMonitor::ovenRemoved, this, &OvenManager::handleOvenProbed);
}

OvenManager::~OvenManager()
{
	stop();
	_udevMonitor->wait();
}

void OvenManager::start()
{
	_udevMonitor->start();
}

void OvenManager::stop()
{
	_udevMonitor->terminate();
}

void OvenManager::setFilamentsEnabled(bool enabled)
{
	if (enabled != _filamentsEnabled) {
		struct usbdevfs_ioctl wrapper;
		wrapper.ifno = 0; // TODO: Not sure if this is right
		wrapper.ioctl_code = enabled ? PCB_OVEN_ENABLE_FILAMENTS :
		                               PCB_OVEN_DISABLE_FILAMENTS;
		wrapper.data = NULL;

		if (ioctl(_ioctlFd, USBDEVFS_IOCTL, &wrapper))
			emit errorOccurred(errno);
		else
			_filamentsEnabled = enabled;
	}
}

void OvenManager::setTargetTemperature(int temperature)
{
	if (temperature != _targetTemperature) {
		struct usbdevfs_ioctl wrapper;
		wrapper.ifno = 0; // TODO: Not sure if this is right
		wrapper.ioctl_code = PCB_OVEN_SET_TEMPERATURE;
		wrapper.data = &temperature;

		if (ioctl(_ioctlFd, USBDEVFS_IOCTL, &wrapper))
			emit errorOccurred(errno);
		else
			_targetTemperature = temperature;
	}
}

void OvenManager::handleOvenProbed()
{
	_ioctlFd = open("/dev/pcboven", O_RDWR, O_NONBLOCK);
	if (_ioctlFd >= 0)
		emit connected();
	else
		emit errorOccurred(errno);
}

void OvenManager::handleOvenRemoved()
{
	close(_ioctlFd);

	emit disconnected();
}

