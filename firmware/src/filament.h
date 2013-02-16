#ifndef __FILAMENT_H__
#define __FILAMENT_H__

#include <stdbool.h>
#include <stdint.h>

struct filament {
	volatile uint8_t *port;
	uint8_t pin;
	bool on;
};

void filament_turn_on(struct filament *filament);
void filament_turn_off(struct filament *filament);

#endif // __FILAMENT_H__

