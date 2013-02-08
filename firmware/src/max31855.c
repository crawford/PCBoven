#include <avr/io.h>
#include <util/delay.h>
#include <errno.h>
#include "max31855.h"

#define CS_PORT    PORTF
#define CS_PIN     (1 << 4)
#define CLK_PORT   PORTF
#define CLK_PIN    (1 << 5)
#define DATA_PORT  PINF
#define DATA_PIN   (1 << 2)

#define PROBE_TEMP_MASK        0xFFFC0000
#define PROBE_TEMP_OFFSET      18
#define FAULT_MASK             0x00010000
#define FAULT_OFFSET           16
#define INTERNAL_TEMP_MASK     0x0000FFF0
#define INTERNAL_TEMP_OFFSET   4
#define SHORT_VCC_MASK         0x00000004
#define SHORT_VCC_OFFSET       2
#define SHORT_GND_MASK         0x00000002
#define SHORT_GND_OFFSET       1
#define OPEN_CIRCUIT_MASK      0x00000001
#define OPNE_CIRCUIT_OFFSET    0

static inline uint32_t spi_read_long();

void max31855_init()
{
	DDRF = DATA_PIN;
	PORTF = 0x00;
}

int max31855_read(struct max31855_result *result)
{
	uint32_t data = spi_read_long();

	if (data & FAULT_MASK)
		return MAX_31855_FAULT;

	result->probe_temp    = (data & PROBE_TEMP_MASK)    << PROBE_TEMP_OFFSET;
	result->internal_temp = (data & INTERNAL_TEMP_MASK) << INTERNAL_TEMP_OFFSET;
	result->short_vcc     = (data & SHORT_VCC_MASK)     << SHORT_VCC_OFFSET;
	result->short_gnd     = (data & SHORT_GND_MASK)     << SHORT_GND_OFFSET;
	result->open_circuit  = (data & OPEN_CIRCUIT_MASK)  << OPNE_CIRCUIT_OFFSET;

	return 0;
}

static inline uint32_t spi_read_long()
{
	uint32_t data = 0;

	CLK_PORT &= ~CLK_PIN;
	CS_PORT  &= ~CS_PIN;

	for (char count = 32; count > 0; count--) {
		CLK_PORT |= CLK_PIN;
		data = (data << 1) | !!(DATA_PORT & DATA_PIN);
		CLK_PORT &= ~CLK_PIN;
	}

	CS_PORT |= CS_PIN;
	return data;
}

