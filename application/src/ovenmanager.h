#ifndef OVENMANAGER_H
#define OVENMANAGER_H

#include <libusb-1.0/libusb.h>
#include <signal.h>
#include <QThread>
#include <QTime>

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
Q_DECLARE_METATYPE(struct oven_state)

class OvenManager : public QThread
{
	Q_OBJECT

	public:
		explicit OvenManager(QObject *parent = 0);
		virtual ~OvenManager();
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

	protected:
		void run();

	private:
		int connectToDevice(libusb_device_handle **handle);
		void sigio_handler(int sig);
		static void LIBUSB_CALL irq_handler(struct libusb_transfer *transfer);

		int _targetTemperature;
		bool _filamentsEnabled;
		bool _connected;
		volatile bool _shouldRun;
		libusb_device_handle *_handle;
		struct libusb_transfer *_irqTransfer;
};

#endif // OVENMANAGER_H

