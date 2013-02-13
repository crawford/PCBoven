#ifndef __MAX31855_H__
#define __MAX31855_H__

#define MAX_31855_FAULT 1

struct max31855_result {
	uint16_t probe_temp;
	uint16_t internal_temp;
	uint8_t short_vcc;
	uint8_t short_gnd;
	uint8_t open_circuit;
};

void max31855_init();
int max31855_read(struct max31855_result *result);

#endif // __MAX31855_H__
