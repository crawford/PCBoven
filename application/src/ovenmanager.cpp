#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <errno.h>
#include <QThread>
#include "ovenmanager.h"

static OvenManager *_sigio_receiver;

OvenManager::OvenManager(QObject *parent) : QObject(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;
}

OvenManager::~OvenManager()
{
	stop();
}

void OvenManager::start()
{
	_ioctlFd = open("/dev/pcboven", O_RDWR, O_NONBLOCK | FASYNC);
	if (_ioctlFd < 0) {
		emit errorOccurred(errno);
		return;
	}

	register_sigio_receiver(this);

	if (fcntl(_ioctlFd, F_SETOWN, getpid())) {
		emit errorOccurred(errno);
		return;
	}

	if (fcntl(_ioctlFd, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | FASYNC)) {
		emit errorOccurred(errno);
		return;
	}

	int ret = ioctl(_ioctlFd, PCBOVEN_IS_CONNECTED);
	if (ret < 0)
		emit errorOccurred(ret);

	if (ret) {
		_connected = true;
		emit connected();
	}
}

void OvenManager::stop()
{
	close(_ioctlFd);
}

void OvenManager::setFilamentsEnabled(bool enabled)
{
	if (enabled != _filamentsEnabled) {
		int code = enabled ? PCBOVEN_ENABLE_FILAMENTS :
		                     PCBOVEN_DISABLE_FILAMENTS;
		if (ioctl(_ioctlFd, code))
			emit errorOccurred(errno);
		else
			_filamentsEnabled = enabled;
	}
}

void OvenManager::setTargetTemperature(int temperature)
{
	if (temperature != _targetTemperature) {
		if (ioctl(_ioctlFd, PCBOVEN_SET_TEMPERATURE))
			emit errorOccurred(errno);
		else
			_targetTemperature = temperature;
	}
}

void OvenManager::sigio_handler(int sig)
{
	QTime timestamp = QTime::currentTime();
	struct oven_state state;
	int ret;
	(void)sig;

	ret = ioctl(_ioctlFd, PCBOVEN_GET_STATE, &state);
	switch (ret) {
	// Oven was disconnected
	case -ENODEV:
		_connected = false;
		close(_ioctlFd);
		emit disconnected();
		break;

	// ioctl syscall error
	case -1:
		emit errorOccurred(errno);
		break;

	// success
	case 0:
		emit readingsRead(state, timestamp);
		break;

	// driver error
	default:
		emit errorOccurred(ret);
	}

}

void OvenManager::top_sigio_handler(int sig)
{
	if (_sigio_receiver)
		_sigio_receiver->sigio_handler(sig);
	signal(SIGIO, &OvenManager::top_sigio_handler);
}

void OvenManager::register_sigio_receiver(OvenManager *receiver)
{
	_sigio_receiver = receiver;
	if (receiver)
		signal(SIGIO, &OvenManager::top_sigio_handler);
	else
		signal(SIGIO, NULL);
}

