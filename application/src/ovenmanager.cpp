#include <errno.h>
#include "ovenmanager.h"

#define PCBOVEN_ID_VENDOR      0x03EB
#define PCBOVEN_ID_PRODUCT     0x3140

#define REQUEST_TIMEOUT_MS     3000

enum control_requests {
	CONTROL_REQUEST_SET_TEMPERATURE = 0x00,
	CONTROL_REQUEST_SET_FILAMENT    = 0x01,
};

#if defined Q_OS_WIN
// uint8_t and int16_t are typedef'd by libusb.h
#pragma pack (1)
struct oven_state_frame {
#elif defined Q_OS_LINUX
struct __attribute((__packed__)) oven_state_frame {
#endif
	int16_t probe;
	int16_t internal;
	uint8_t short_vcc;
	uint8_t short_gnd;
	uint8_t open_circuit;
	uint8_t top_on;
	uint8_t bottom_on;
};
#if defined Q_OS_WIN
#pragma pack ()
#endif

static inline int16_t _temperature12_to_data(int temperature);
static inline int _data_to_temperature12(int16_t data);
static inline int16_t _temperature14_to_data(int temperature);
static inline int _data_to_temperature14(int16_t data);

OvenManager::OvenManager(QObject *parent) : QThread(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;

	qRegisterMetaType<struct oven_state>("oven_state");
	_irqBuffer = new unsigned char[sizeof(struct oven_state_frame)]();

	libusb_init(NULL);
	libusb_set_debug(NULL, 100);
}

OvenManager::~OvenManager()
{
	stop();
	libusb_exit(NULL);
	delete _irqBuffer;
}

void LIBUSB_CALL OvenManager::irq_handler(struct libusb_transfer *transfer)
{
	OvenManager *manager = (OvenManager *)transfer->user_data;
	struct oven_state_frame *frame = (struct oven_state_frame *)(transfer->buffer);
	struct oven_state state;
	int ret;

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED)
		return;

	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		manager->_shouldRun = false;
		emit manager->errorOccurred("communication with oven lost");
		return;
	}

	state.probe_temp         = _data_to_temperature14(frame->probe);
	state.internal_temp      = _data_to_temperature12(frame->internal);
	state.fault_short_vcc    = frame->short_vcc;
	state.fault_short_gnd    = frame->short_gnd;
	state.fault_open_circuit = frame->open_circuit;
	state.filament_top_on    = frame->top_on;
	state.filament_bottom_on = frame->bottom_on;

	emit manager->readingsRead(state, QTime::currentTime());

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
			_irqBuffer,
			sizeof(_irqBuffer),
			&irq_handler,
			this,
			REQUEST_TIMEOUT_MS);
	ret = libusb_submit_transfer(_irqTransfer);
	if (ret)
	{
		emit errorOccurred(libusb_error_name(ret));
		return;
	}

	_shouldRun = true;
	while (_shouldRun)
	{
		struct timeval timeout = { 1, 0 };
		libusb_handle_events_timeout_completed(NULL, &timeout, NULL);
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
				_temperature14_to_data(temperature),
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

static inline int16_t _temperature12_to_data(int temperature)
{
	// Temperature12 is a 12-bit value with where the LSB is 2^-4
	return (temperature << 4);
}

static inline int _data_to_temperature12(int16_t data)
{
	// Temperature12 is a 12-bit value with where the LSB is 2^-4
	return libusb_le16_to_cpu(data << 4) >> 8;
}

static inline int16_t _temperature14_to_data(int temperature)
{
	// Temperature14 is a 14-bit value with where the LSB is 2^-2
	return (temperature << 2);
}

static inline int _data_to_temperature14(int16_t data)
{
	// Temperature14 is a 14-bit value with where the LSB is 2^-2
	return libusb_le16_to_cpu(data << 2) >> 4;
}

