#include <QThread>
#include "ioctlworker.h"

IoctlWorker::IoctlWorker(QObject *parent) : QObject(parent)
{
}

void IoctlWorker::sendSettings(bool enabled, int temperature)
{
	(void)enabled;
	(void)temperature;
	QThread::currentThread()->sleep(1);
	emit settingsSent();
}
