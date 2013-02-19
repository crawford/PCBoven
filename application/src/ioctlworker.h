#ifndef IOCTLWORKER_H
#define IOCTLWORKER_H

#include <QObject>

class IoctlWorker : public QObject
{
	Q_OBJECT

	public:
		explicit IoctlWorker(QObject *parent = 0);

	signals:
		void settingsSent();

	public slots:
		void sendSettings(bool enabled, int temperature);

};

#endif // IOCTLWORKER_H
