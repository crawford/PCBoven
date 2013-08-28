#include <errno.h>
#include <QDebug>
#include "ovenmanager.h"

#define PCBOVEN_ID_VENDOR      0x03EB
#define PCBOVEN_ID_PRODUCT     0x3140

#define REQUEST_TIMEOUT_MS     3000

static void _register_sigio_receiver(OvenManager *receiver);

static OvenManager *_sigio_receiver;

enum control_requests {
	CONTROL_REQUEST_SET_TEMPERATURE = 0x00,
	CONTROL_REQUEST_SET_FILAMENT    = 0x01,
};

#if defined(_Windows)
	typedef int16_t INT16;
	typedef uint8_t UINT8;
#define PACKED # pragma pack (1)
#define UNPACKED # pragma pack ()
#define __attribute__()
#else
#define PACKED
#define UNPACKED
#endif

PACKED
struct __attribute__ ((__packed__)) oven_state_frame {
	int16_t probe;
	int16_t internal;
	uint8_t short_vcc;
	uint8_t short_gnd;
	uint8_t open_circuit;
	uint8_t top_on;
	uint8_t bottom_on;
};
UNPACKED

OvenManager::OvenManager(QObject *parent) : QThread(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;

	qRegisterMetaType<struct oven_state>("oven_state");

	libusb_init(NULL);
	libusb_set_debug(NULL, 100);
}

OvenManager::~OvenManager()
{
	stop();
	libusb_exit(NULL);
}

static unsigned char irqbuf[sizeof(struct oven_state_frame)];
void LIBUSB_CALL OvenManager::irq_handler(struct libusb_transfer *transfer)
{
	OvenManager *manager = (OvenManager *)transfer->user_data;
	struct oven_state_frame *frame = (struct oven_state_frame *)(transfer->buffer);
	struct oven_state state;
	int ret;

	qDebug() << "irq transfer status " << transfer->status;

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED)
		return;

	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		manager->_shouldRun = false;
		emit manager->errorOccurred("communication with oven lost");
		return;
	}

	state.probe_temp         = (libusb_le16_to_cpu(frame->probe) << 2) >> 4;
	state.internal_temp      = (libusb_le16_to_cpu(frame->internal) << 4) >> 8;
	state.fault_short_vcc    = frame->short_vcc;
	state.fault_short_gnd    = frame->short_gnd;
	state.fault_open_circuit = frame->open_circuit;
	state.filament_top_on    = frame->top_on;
	state.filament_bottom_on = frame->bottom_on;

	emit manager->readingsRead(state, QTime::currentTime());

	for (unsigned int i = 0; i < sizeof(irqbuf); i++)
		qDebug() << "IRQ callback " << transfer->buffer[i];

	ret = libusb_submit_transfer(transfer);
	if (ret)
	{
		manager->_shouldRun = false;
		emit manager->errorOccurred(libusb_error_name(ret));
	}
}

void OvenManager::run()
{
	int ret = connectToDevice(&_handle);
	_connected = false;
	if (ret)
		emit errorOccurred(libusb_error_name(ret));
	else if (!_handle)
		return;

	_connected = true;
	emit connected();

	_irqTransfer = libusb_alloc_transfer(0);
	if (!_irqTransfer)
	{
		emit errorOccurred("libusb_alloc_transfer()");
		return;
	}
	libusb_fill_interrupt_transfer(
			_irqTransfer,
			_handle,
			LIBUSB_ENDPOINT_IN | 1,
			irqbuf,
			sizeof(irqbuf),
			&irq_handler,
			this,
			REQUEST_TIMEOUT_MS);
	ret = libusb_submit_transfer(_irqTransfer);
	if (ret)
	{
		emit errorOccurred(libusb_error_name(ret));
		return;
	}

	_register_sigio_receiver(this);

	_shouldRun = true;
	while (_shouldRun)
	{
		struct timeval timeout = { 1, 0 };
		libusb_handle_events_timeout_completed(NULL, &timeout, NULL);
		qDebug() << "Looping";
	}
}

void OvenManager::stop()
{
	if (isRunning())
	{
		libusb_cancel_transfer(_irqTransfer);
		_shouldRun = false;
		wait();
	}
	libusb_free_transfer(_irqTransfer);

	if (_handle)
		libusb_close(_handle);
}

void OvenManager::setFilamentsEnabled(bool enabled)
{
	if (enabled != _filamentsEnabled) {
		int ret = libusb_control_transfer(
				_handle,
				LIBUSB_REQUEST_TYPE_VENDOR,
				CONTROL_REQUEST_SET_FILAMENT,
				enabled,
				0,
				NULL,
				0,
				REQUEST_TIMEOUT_MS);
		if (ret)
			emit errorOccurred(libusb_error_name(ret));
		else
			_filamentsEnabled = enabled;
	}
}

void OvenManager::setTargetTemperature(int temperature)
{
	if (temperature != _targetTemperature) {
		int ret = libusb_control_transfer(
				_handle,
				LIBUSB_REQUEST_TYPE_VENDOR,
				CONTROL_REQUEST_SET_TEMPERATURE,
				temperature,
				0,
				NULL,
				0,
				REQUEST_TIMEOUT_MS);
		if (ret)
			emit errorOccurred(libusb_error_name(ret));
		else
			_targetTemperature = temperature;
	}
}

int OvenManager::connectToDevice(libusb_device_handle **handle)
{
	int ret;
	libusb_device **devices;

	*handle = NULL;

	ret = libusb_get_device_list(NULL, &devices);
	if (ret < 0)
		goto cleanup;

	for (libusb_device **device = devices; *device; device++)
	{
		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(*device, &desc);
		if (ret)
			goto cleanup;

		if (desc.idVendor  == PCBOVEN_ID_VENDOR &&
		    desc.idProduct == PCBOVEN_ID_PRODUCT)
		{
			ret = libusb_open(*device, handle);
			if (ret)
				goto cleanup;

			ret = libusb_claim_interface(*handle, 0);
			if (ret)
				goto cleanup;
			else
				break;
		}
	}

cleanup:
	libusb_free_device_list(devices, true);

	return ret;
}

void OvenManager::sigio_handler(int sig)
{
	QTime timestamp = QTime::currentTime();
	struct oven_state state;
	int ret;
	(void)sig;

	if (_connected) {
		ret = -2;//ioctl(_ioctlFd, PCBOVEN_GET_STATE, &state);
		if (ret == 0) {
			emit readingsRead(state, timestamp);
		} else if (ret == -1) {
			if (errno == ENODEV) {
				_connected = false;
				emit disconnected();
			} else {
				emit errorOccurred("TODO");
			}
		} else {
			emit errorOccurred("TODO");
		}
	} else {
		ret = -1;//ioctl(_ioctlFd, PCBOVEN_IS_CONNECTED, &state);
		if (ret < 0) {
			emit errorOccurred("TODO");
		} else if (ret) {
			_connected = true;
			emit connected();
		}
	}
}

void OvenManager::top_sigio_handler(int sig)
{
	if (_sigio_receiver)
		_sigio_receiver->sigio_handler(sig);
	signal(SIGIO, &OvenManager::top_sigio_handler);
}

static void _register_sigio_receiver(OvenManager *receiver)
{
	_sigio_receiver = receiver;
	if (receiver)
		signal(SIGIO, &OvenManager::top_sigio_handler);
	else
		signal(SIGIO, NULL);
}

