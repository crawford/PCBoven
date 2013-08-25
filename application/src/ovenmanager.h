#ifndef OVENMANAGER_H
#define OVENMANAGER_H

#include <libusb-1.0/libusb.h>
#include <signal.h>
#include <QTime>

#define PCBOVEN_ID_VENDOR      0x03EB
#define PCBOVEN_ID_PRODUCT     0x3140

struct oven_state {
	int probe_temp;
	int internal_temp;
	int target_temp;
	bool enable_filaments;
	bool fault_short_vcc;
	bool fault_short_gnd;
	bool fault_open_circuit;
	bool filament_top_on;
	bool filament_bottom_on;
};

class OvenManager : public QObject
{
	Q_OBJECT

	public:
		explicit OvenManager(QObject *parent = 0);
		virtual ~OvenManager();
		void start();
		void stop();
		static void top_sigio_handler(int signal);

	signals:
		void connected();
		void disconnected();
		void readingsRead(struct oven_state readings, QTime timestamp);
		void errorOccurred(QString error);

	public slots:
		void setFilamentsEnabled(bool enabled);
		void setTargetTemperature(int temperature);

	private:
		int connectToDevice(libusb_device_handle **handle);
		void sigio_handler(int sig);

		int _targetTemperature;
		bool _filamentsEnabled;
		bool _connected;
		libusb_device_handle *_handle;
};

#endif // OVENMANAGER_H

