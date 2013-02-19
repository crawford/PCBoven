#include "udevmonitor.h"

UdevMonitor::UdevMonitor(QObject *parent) : QThread(parent) {
}

void UdevMonitor::run() {
	QThread::currentThread()->sleep(2);
	emit connected();
}

