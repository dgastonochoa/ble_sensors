#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

typedef union 
{
    uint32_t u32;
    int32_t i32;
    uint16_t u16;
} atomic_val_t;

typedef struct 
{
    atomic_val_t v;
} atomic_t;

atomic_val_t atomic_get(const atomic_t* atm);

void atomic_set(atomic_t* atm, atomic_val_t val);

#endif /* ATOMIC_H */