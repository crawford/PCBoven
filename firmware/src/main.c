#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include "descriptors.h"
#include "max31855.h"
#include "filament.h"

#define TEMP_READ_RATE 1
#define FILAMENT_TOP_PORT    &PORTB
#define FILAMENT_TOP_PIN     2
#define FILAMENT_BOTTOM_PORT &PORTB
#define FILAMENT_BOTTOM_PIN  5

void platform_init();
int process_reading(struct max31855_result reading, int16_t target);

volatile bool g_take_readings;

int main()
{
	int16_t target_probe_temp = 0;
	bool filaments_enabled = false;
	struct max31855_result reading;
	struct filament top_filament =
	{
		.port = FILAMENT_TOP_PORT,
		.pin  = FILAMENT_TOP_PIN
	};
	struct filament bottom_filament =
	{
		.port = FILAMENT_BOTTOM_PORT,
		.pin  = FILAMENT_BOTTOM_PIN
	};
	int result;

	platform_init();
	max31855_init();
	USB_Init();
	LEDs_Init();

	g_take_readings = false;
	sei();

	while (true) {
		Endpoint_SelectEndpoint(OUT_EPNUM);
		if (Endpoint_IsOUTReceived()) {
			target_probe_temp = Endpoint_Read_16_LE();
			filaments_enabled = !!Endpoint_Read_8();

			if (!filaments_enabled) {
				filament_turn_off(&top_filament);
				filament_turn_off(&bottom_filament);
			}

			Endpoint_ClearOUT();
		}
		if (g_take_readings) {
			g_take_readings = false;

			Endpoint_SelectEndpoint(IN_EPNUM);

			if (max31855_read(&reading)) {
				LEDs_ToggleLEDs(LEDS_ALL_LEDS);
			}

			result = process_reading(reading, target_probe_temp);
			if (result > 0)
				filament_turn_on(&top_filament);
			else
				filament_turn_off(&top_filament);

			if (result > 1)
				filament_turn_on(&bottom_filament);
			else
				filament_turn_off(&bottom_filament);

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

