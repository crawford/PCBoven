#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include "descriptors.h"
#include "max31855.h"
#include "filament.h"

#define TEMP_READ_RATE 1
#define FILAMENT_TOP_PORT    PORTF
#define FILAMENT_TOP_PIN     0
#define FILAMENT_BOTTOM_PORT PORTF
#define FILAMENT_BOTTOM_PIN  1

enum control_requests {
	CONTROL_REQUEST_SET_TEMPERATURE = 0x00,
	CONTROL_REQUEST_SET_FILAMENT    = 0x01,
};

void platform_init();
int process_reading(struct max31855_result reading, int16_t target);

volatile bool g_take_readings;
volatile bool filaments_enabled;
volatile int16_t target_probe_temp;

int main()
{
	struct max31855_result reading;
	struct filament top_filament =
	{
		.port = &FILAMENT_TOP_PORT,
		.pin  = FILAMENT_TOP_PIN,
		.on   = false
	};
	struct filament bottom_filament =
	{
		.port = &FILAMENT_BOTTOM_PORT,
		.pin  = FILAMENT_BOTTOM_PIN,
		.on   = false
	};
	int result;

	platform_init();
	max31855_init();
	USB_Init();
	LEDs_Init();

	set_sleep_mode(SLEEP_MODE_IDLE);

	g_take_readings = false;
	sei();

	while (true) {
		if (!filaments_enabled) {
			filament_turn_off(&top_filament);
			filament_turn_off(&bottom_filament);
		}

		if (g_take_readings) {
			g_take_readings = false;

			if (max31855_read(&reading)) {
				LEDs_ToggleLEDs(LEDS_ALL_LEDS);
				filament_turn_off(&top_filament);
				filament_turn_off(&bottom_filament);
			} else {
				result = process_reading(reading, target_probe_temp);
				if (filaments_enabled) {
					if (result > 0)
						filament_turn_on(&top_filament);
					else
						filament_turn_off(&top_filament);

					if (result > 1)
						filament_turn_on(&bottom_filament);
					else
						filament_turn_off(&bottom_filament);
				}
			}

			Endpoint_SelectEndpoint(IN_EPADDR);

			Endpoint_Write_16_LE(reading.probe_temp);
			Endpoint_Write_16_LE(reading.internal_temp);
			Endpoint_Write_8(reading.short_vcc);
			Endpoint_Write_8(reading.short_gnd);
			Endpoint_Write_8(reading.open_circuit);
			Endpoint_Write_8(top_filament.on);
			Endpoint_Write_8(bottom_filament.on);

			Endpoint_ClearIN();
		}
		USB_USBTask();
		//sleep_mode();
	}

	__builtin_unreachable();
}

void EVENT_USB_Device_ControlRequest()
{
	if (((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR) &&
	    ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_DEVICE))
	{
		Endpoint_ClearSETUP();
		if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == REQDIR_HOSTTODEVICE)
		{
			switch(USB_ControlRequest.bRequest)
			{
				case CONTROL_REQUEST_SET_FILAMENT:
					filaments_enabled = USB_ControlRequest.wValue;
					break;
				case CONTROL_REQUEST_SET_TEMPERATURE:
					target_probe_temp = USB_ControlRequest.wValue;
					break;
			}
		}
		Endpoint_ClearStatusStage();
	}
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
}

int process_reading(struct max31855_result reading, int16_t target)
{
	if (reading.probe_temp * 2 < target)
		return 2;

	if (reading.probe_temp < target)
		return 1;

	return 0;
}

ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
	g_take_readings = true;
}

