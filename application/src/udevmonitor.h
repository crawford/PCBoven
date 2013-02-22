#ifndef UDEVMONITOR_H
#define UDEVMONITOR_H

#include <QThread>

class UdevMonitor : public QThread
{
	Q_OBJECT

	public:
		explicit UdevMonitor(QObject *parent = 0);

	signals:
		void ovenProbed();
		void ovenRemoved();

	protected:
		void run();
};

#endif // UDEVMONITOR_H

