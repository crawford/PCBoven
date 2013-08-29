#ifndef __DESCRIPTORS_H__
#define __DESCRIPTORS_H__

#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>


#define IN_EPADDR    (ENDPOINT_DIR_IN | 1)
#define IN_EPSIZE    16

/** Type define for the device configuration descriptor structure. This must be defined in the
 *  application code, as the configuration descriptor contains several sub-descriptors which
 *  vary between devices, and which describe the device's usage to the host.
 *
 *  NOTE: Do not modify this structure. LUFA writes this out in byte order to the host!
 */
typedef struct {
	USB_Descriptor_Configuration_Header_t Config;
	USB_Descriptor_Interface_t            Interface;
	USB_Descriptor_Endpoint_t             DataInEndpoint;
} USB_Descriptor_Configuration_t;

#endif // __DESCRIPTORS_H__

