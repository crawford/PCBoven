#ifndef OVENMANAGER_H
#define OVENMANAGER_H

#include <signal.h>
#include <QTime>
#include "pcboven_usb.h"

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
		void readingsRead(struct oven_state readings, QTime timestamp);
		void errorOccurred(int error);

	public slots:
		void setFilamentsEnabled(bool enabled);
		void setTargetTemperature(int temperature);

	protected:
		static void register_sigio_receiver(OvenManager *receiver);
		static void top_sigio_handler(int signal);

	private:
		void sigio_handler(int sig);

		int _targetTemperature;
		bool _filamentsEnabled;
		bool _connected;
		int _ioctlFd;
};

#endif // OVENMANAGER_H

