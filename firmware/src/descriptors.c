#include "descriptors.h"

/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
	.Header                 = { .Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device },
	.USBSpecification       = VERSION_BCD(02.00),

	.Class                  = 0x00,
	.SubClass               = 0x00,
	.Protocol               = 0x00,

	.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

	.VendorID               = 0x03EB,
	.ProductID              = 0x3140,
	.ReleaseNumber          = VERSION_BCD(00.01),
	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex        = 0x02,
	.SerialNumStrIndex      = USE_INTERNAL_SERIAL,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
	.Config = {
		.Header                 = { .Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},
		.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces        = 1,
		.ConfigurationNumber    = 1,
		.ConfigurationStrIndex  = NO_DESCRIPTOR,
		.ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),
		.MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
	},

	.Interface = {
		.Header           = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},
		.InterfaceNumber  = 0,
		.AlternateSetting = 0,
		.TotalEndpoints   = 1,
		.Class            = 0x00,
		.SubClass         = 0x00,
		.Protocol         = 0x00,
		.InterfaceStrIndex = NO_DESCRIPTOR
	},

	.DataInEndpoint = {
		.Header            = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
		.EndpointAddress   = IN_EPNUM,
		.Attributes        = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize      = IN_EPSIZE,
		.PollingIntervalMS = 5
	},
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString = {
	.Header                 = { .Size = USB_STRING_LEN(1), .Type = DTYPE_String },
	.UnicodeString          = { LANGUAGE_ID_ENG }
};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString = {
	.Header                 = { .Size = USB_STRING_LEN(8), .Type = DTYPE_String },
	.UnicodeString          = L"Crawford"
};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString = {
	.Header                 = { .Size = USB_STRING_LEN(8), .Type = DTYPE_String },
	.UnicodeString          = L"PCBoven™"
};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex, const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	switch (DescriptorType) {
		case DTYPE_Device:
			*DescriptorAddress = &DeviceDescriptor;
			return sizeof(USB_Descriptor_Device_t);

		case DTYPE_Configuration:
			*DescriptorAddress = &ConfigurationDescriptor;
			return sizeof(USB_Descriptor_Configuration_t);

		case DTYPE_String:
			switch (DescriptorNumber) {
				case 0x00:
					*DescriptorAddress = &LanguageString;
					return pgm_read_byte(&LanguageString.Header.Size);

				case 0x01:
					*DescriptorAddress = &ManufacturerString;
					return pgm_read_byte(&ManufacturerString.Header.Size);

				case 0x02:
					*DescriptorAddress = &ProductString;
					return pgm_read_byte(&ProductString.Header.Size);
			}

			break;
	}

	return NO_DESCRIPTOR;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged() {
	Endpoint_ConfigureEndpoint(IN_EPNUM, EP_TYPE_INTERRUPT, IN_EPSIZE, 1);
}

