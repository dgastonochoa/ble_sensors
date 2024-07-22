#include "atomic.h"

#include "freertos/FreeRTOS.h"

static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

atomic_val_t atomic_get(const atomic_t* atm)
{
    atomic_val_t tmp;
    portENTER_CRITICAL(&spinlock);
    tmp = atm->v;
    portEXIT_CRITICAL(&spinlock);
    return tmp;
}

void atomic_set(atomic_t* atm, atomic_val_t val)
{
    portENTER_CRITICAL(&spinlock);
    atm->v = val;
    portEXIT_CRITICAL(&spinlock);
}
