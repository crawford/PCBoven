#include <QThread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "ioctlworker.h"

#define PCB_OVEN_ENABLE_FILAMENTS  -1
#define PCB_OVEN_DISABLE_FILAMENTS -1
#define PCB_OVEN_SET_TEMPERATURE   -1

IoctlWorker::IoctlWorker(QObject *parent) : QObject(parent)
{
}

IoctlWorker::~IoctlWorker()
{
	close(_ioctlFd);
}

void IoctlWorker::initialize()
{
	_ioctlFd = open("/dev/bus/usb/001", O_RDONLY, O_NONBLOCK);
	if (_ioctlFd < 0)
		emit errorOccurred(errno);
}

void IoctlWorker::setTemperature(int temperature)
{
	(void)temperature;
	QThread::currentThread()->sleep(1);
	if (ioctl(_ioctlFd, PCB_OVEN_SET_TEMPERATURE))
		emit errorOccurred(errno);
}

void IoctlWorker::enableFilaments(bool enabled)
{
	int request = enabled ? PCB_OVEN_ENABLE_FILAMENTS :
	                        PCB_OVEN_DISABLE_FILAMENTS;

	QThread::currentThread()->sleep(1);
	if (ioctl(_ioctlFd, request))
		emit errorOccurred(errno);
}

void IoctlWorker::waitForReadings()
{

}

