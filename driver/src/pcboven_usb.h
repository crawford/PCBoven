#ifndef __PCBOVEN_USB_H__
#define __PCBOVEN_USB_H__

#include <linux/ioctl.h>

#define PCBOVEN_USB_ID_VENDOR      0x03EB
#define PCBOVEN_USB_ID_PRODUCT     0x3140

#define PCBOVEN_IOCTL_MAGIC        0xA1
#define PCBOVEN_MISC_MINOR         0x54

#define PCBOVEN_IS_CONNECTED       _IOR(PCBOVEN_IOCTL_MAGIC, 'C', int)
#define PCBOVEN_GET_STATE          _IOR(PCBOVEN_IOCTL_MAGIC, 'S', struct oven_state)
#define PCBOVEN_SET_TEMPERATURE    _IOW(PCBOVEN_IOCTL_MAGIC, 'T', int)
#define PCBOVEN_ENABLE_FILAMENTS   _IO(PCBOVEN_IOCTL_MAGIC, 'E')
#define PCBOVEN_DISABLE_FILAMENTS  _IO(PCBOVEN_IOCTL_MAGIC, 'D')

struct oven_state {
	int16_t probe_temp;
	int16_t internal_temp;
	int16_t target_temp;
	bool enable_filaments;
	bool fault_short_vcc;
	bool fault_short_gnd;
	bool fault_open_circuit;
	bool filament_top_on;
	bool filament_bottom_on;
};

#endif

