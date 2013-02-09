#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include "descriptors.h"
#include "max31855.h"

#define TEMP_READ_RATE 2

void platform_init();

volatile bool g_take_readings;

int main()
{
	struct max31855_result reading;

	LEDs_Init();
	platform_init();
	max31855_init();
	USB_Init();

	g_take_readings = false;
	sei();

	while (1) {
		Endpoint_SelectEndpoint(OUT_EPNUM);
		if (Endpoint_IsOUTReceived()) {
			Endpoint_ClearOUT();
		}
		if (g_take_readings) {
			g_take_readings = false;
			LEDs_ToggleLEDs(LEDS_ALL_LEDS);

			Endpoint_SelectEndpoint(IN_EPNUM);

			if (max31855_read(&reading)) {
				Endpoint_Write_32_LE('X');
			}

			Endpoint_Write_8('A');
			Endpoint_Write_8('l');
			Endpoint_Write_8('e');
			Endpoint_Write_8('x');
			Endpoint_Write_8(0);

			Endpoint_ClearIN();
		}
		USB_USBTask();
	}

	__builtin_unreachable();
}

void platform_init()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Timer Initialization */
	OCR1A = F_CPU / 1024 / TEMP_READ_RATE;

	TCCR1A = 0;
	TCCR1B = (1 << WGM12 |   // CTC
	          1 << COM1A1 |  // Clear on compare match
	          0x05);         // Set the pre-scaler to 1024
	TIMSK1 = (1 << OCIE1A);  // Enable interrupt at set point

	LEDs_ToggleLEDs(LEDS_ALL_LEDS);
}

ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
	g_take_readings = true;
}

