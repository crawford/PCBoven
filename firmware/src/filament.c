#include "filament.h"

void filament_turn_on(struct filament *filament)
{
	filament->on = true;
	*filament->port |= (1U << filament->pin);
}

void filament_turn_off(struct filament *filament)
{
	filament->on = false;
	*filament->port &= ~(1U << filament->pin);
}

