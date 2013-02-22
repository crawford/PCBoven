#ifndef IOCTLWORKER_H
#define IOCTLWORKER_H

#include <QObject>

class IoctlWorker : public QObject
{
	Q_OBJECT

	public:
		explicit IoctlWorker(QObject *parent = 0);
		virtual ~IoctlWorker();
		void initialize();

	signals:
		void errorOccurred(int error);

	public slots:
		void setTemperature(int temperature);
		void enableFilaments(bool enabled);
		void waitForReadings();

	protected:
		int _ioctlFd;

};

#endif // IOCTLWORKER_H
