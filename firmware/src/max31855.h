#ifndef __MAX31855_H__
#define __MAX31855_H__

#define MAX_31855_FAULT 1

struct max31855_result {
	long probe_temp:14;
	long internal_temp:12;
	char short_vcc:1;
	char short_gnd:1;
	char open_circuit:1;
};

void max31855_init();
int max31855_read(struct max31855_result *result);

#endif // __MAX31855_H__
