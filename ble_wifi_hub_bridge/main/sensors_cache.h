#ifndef SENSORS_CACHE_H
#define SENSORS_CACHE_H

#include <stdint.h>

enum sensor
{
    SENSOR_MAGNETIC_FIELD,
    SENSOR_PHOTOCELL,
    SENSOR_TEMP_DETECTOR,
    SENSOR_IR_DETECTOR,
    SENSOR_NONE
};

typedef union
{
    uint16_t u16;
} sensor_val_t;

/**
 * @brief Thread-safe. Get a sensor's value.
 *
 */
int sensors_cache_get(enum sensor s, sensor_val_t* val);

/**
 * @brief Thread-safe. Set a sensor's value.
 *
 */
int sensors_cache_set(enum sensor s, sensor_val_t val);

#endif /* SENSORS_CACHE_H */
