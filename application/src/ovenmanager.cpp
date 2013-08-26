#include <errno.h>
#include <QThread>
#include "ovenmanager.h"

static void _register_sigio_receiver(OvenManager *receiver);

static OvenManager *_sigio_receiver;

OvenManager::OvenManager(QObject *parent) : QObject(parent)
{
	_filamentsEnabled = false;
	_targetTemperature = 0;
	_connected = false;

	libusb_init(NULL);
	libusb_set_debug(NULL, 3);
}

OvenManager::~OvenManager()
{
	stop();
	libusb_exit(NULL);
}

void OvenManager::start()
{
	int ret = connectToDevice(&_handle);
	_connected = false;
	if (ret)
	{
		emit errorOccurred(libusb_error_name(ret));
	}
	else if (_handle)
	{
		_connected = true;
		emit connected();
	}

	_register_sigio_receiver(this);
}

void OvenManager::stop()
{
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

