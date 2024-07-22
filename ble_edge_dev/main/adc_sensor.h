#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

struct adc_sensor
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_init_cfg;
    adc_oneshot_chan_cfg_t adc_cfg;
    adc_cali_handle_t adc_cali_handle;
    adc_channel_t ch;
    bool initialized;
};

int adc_sensor_init(struct adc_sensor* adc, adc_unit_t unit, adc_channel_t ch);

int adc_sensor_read(struct adc_sensor* adc, uint16_t* res);

#endif /* ADC_SENSOR_H */