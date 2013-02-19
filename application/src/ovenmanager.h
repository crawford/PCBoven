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
		void settingsSent();
		void settings(bool enabled, int temperature);

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
};

#endif // OVENMANAGER_H

