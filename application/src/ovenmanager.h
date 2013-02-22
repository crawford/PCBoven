#ifndef OVENMANAGER_H
#define OVENMANAGER_H

#include <QThread>
#include "udevmonitor.h"
#include "ioctlworker.h"

struct OvenState {
	int probeTemperature;
	int internalTemperature;
	bool faultOpenCircuit;
	bool faultShortToGnd;
	bool faultShortToVcc;
	bool filamentTopOn;
	bool filamentBottomOn;
};

class OvenManager : public QObject
{
	Q_OBJECT

	public:
		explicit OvenManager(QObject *parent = 0);
		virtual ~OvenManager();
		void start();
		void stop();

	signals:
		void connected();
		void disconnected();
		void readingsRead(struct OvenState readings);
		void errorOccurred(int error);
		void setTemperature(int temperature);
		void enableFilaments(bool enabled);

	public slots:
		void setFilamentsEnabled(bool enabled);
		void setTargetTemperature(int temperature);

	private:
		QThread *_ioctlCmdThread;
		QThread *_ioctlReadingThread;
		UdevMonitor *_udevMonitor;
		IoctlWorker *_ioctlWorker;
		int _targetTemperature;
		bool _filamentsEnabled;
		bool _connected;
		int _ioctlFd;

	private slots:
		void handleOvenProbed();
		void handleOvenRemoved();
};

#endif // OVENMANAGER_H

