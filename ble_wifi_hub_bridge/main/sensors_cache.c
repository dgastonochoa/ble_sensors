#include <errno.h>

#include "atomic.h"
#include "sensors_cache.h"

static atomic_t sensor_magnetic_field_val = {0};
static atomic_t sensor_photocell_val = {0};
static atomic_t sensor_temp_val = {0};
static atomic_t sensor_ir_detector_val = {0};

int sensors_cache_set(enum sensor s, sensor_val_t val)
{
    switch (s) {
        case SENSOR_MAGNETIC_FIELD: {
            atomic_set(
                &sensor_magnetic_field_val, (atomic_val_t)val.u16);
            break;
        }

        case SENSOR_PHOTOCELL: {
            atomic_set(
                &sensor_photocell_val, (atomic_val_t)val.u16);
            break;
        }

        case SENSOR_TEMP_DETECTOR: {
            atomic_set(
                &sensor_temp_val, (atomic_val_t)val.u16);
            break;
        }

        case SENSOR_IR_DETECTOR: {
            atomic_set(
                &sensor_ir_detector_val, (atomic_val_t)val.u16);
            break;
        }

        default: {
            return -EINVAL;
        }
    }

    return 0;
}

int sensors_cache_get(enum sensor s, sensor_val_t* val)
{
    switch (s) {
        case SENSOR_MAGNETIC_FIELD: {
            val->u16 = atomic_get(&sensor_magnetic_field_val).u16;
            break;
        }

        case SENSOR_PHOTOCELL: {
            val->u16 = atomic_get(&sensor_photocell_val).u16;
            break;
        }

        case SENSOR_TEMP_DETECTOR: {
            val->u16 = atomic_get(&sensor_temp_val).u16;
            break;
        }

        case SENSOR_IR_DETECTOR: {
            val->u16 = atomic_get(&sensor_ir_detector_val).u16;
            break;
        }

        default: {
            return -EINVAL;
        }
    }

    return 0;
}
