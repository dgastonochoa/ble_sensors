#include <assert.h>

#include "adc_sensor.h"

int adc_sensor_init(struct adc_sensor* adc, adc_unit_t unit, adc_channel_t ch)
{
    adc->adc_init_cfg.unit_id = unit;
    int rc = adc_oneshot_new_unit(&adc->adc_init_cfg, &adc->adc_handle);
    if (rc != 0) {
        return rc;
    }

    adc->ch = ch;
    adc->adc_cfg.bitwidth = ADC_BITWIDTH_12;
    adc->adc_cfg.atten = ADC_ATTEN_DB_11;
    rc = adc_oneshot_config_channel(adc->adc_handle, adc->ch, &adc->adc_cfg);
    if (rc != 0) {
        return rc;
    }

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adc->adc_init_cfg.unit_id,
        .atten = adc->adc_cfg.atten,
        .bitwidth = adc->adc_cfg.bitwidth,
    };
    rc = adc_cali_create_scheme_line_fitting(
        &cali_config, &adc->adc_cali_handle);
    if (rc != 0) {
        return rc;
    }

    adc->initialized = true;

    return rc;
}

int adc_sensor_read(struct adc_sensor* adc, uint16_t* res)
{
    int raw = 0;
    int rc = adc_oneshot_read(adc->adc_handle, adc->ch, &raw);
    if (rc != 0) {
        return rc;
    }

    int voltage = 0;
    rc = adc_cali_raw_to_voltage(adc->adc_cali_handle, raw, &voltage);
    if (rc != 0) {
        return rc;
    }

    assert(adc->adc_cfg.bitwidth == ADC_BITWIDTH_12);
    *res = voltage & 0xfff;

    return rc;
}
