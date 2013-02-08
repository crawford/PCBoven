#include <avr/io.h>
#include <util/delay.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include "descriptors.h"
#include "max31855.h"

// Needs a timer
volatile bool g_take_readings;

int main()
{
	struct max31855_result reading;

	max31855_init();
	LEDs_Init();
	USB_Init();

	g_take_readings = false;

	while (1) {
		Endpoint_SelectEndpoint(OUT_EPNUM);
		if (Endpoint_IsOUTReceived()) {
			Endpoint_ClearOUT();
		}
		if (g_take_readings) {
			g_take_readings = false;
			LEDs_ToggleLEDs(LEDS_LED1);

			Endpoint_SelectEndpoint(IN_EPNUM);

			if (max31855_read(&reading)) {
				Endpoint_Write_32_LE(57);
			}

			Endpoint_Write_32_LE(0);
			Endpoint_Write_32_LE(1);
			Endpoint_Write_32_LE(2);

			Endpoint_ClearIN();
		}
		USB_USBTask();
	}

	__builtin_unreachable();
}

