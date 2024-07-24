def _adc_parse_read_val_to_voltage(adc_raw_read_val: int) -> float:
    '''
    The sensor reads sent by the ble_wifi_bridge device are the raw ADC
    reads; they need to be parsed to actual voltages.

    See https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/adc.html#adc-conversion

    '''
    adc_volt_range_atten_db_11 = [150e-3, 2450e-3]
    adc_max_output_result = 4095

    return (adc_raw_read_val * adc_volt_range_atten_db_11[1] /
            adc_max_output_result)


def _sens_photocell_estimate_resistance(photocell_raw_read_val: int) -> int:
    '''
    Estimate the photocell current resistance value from its raw read value.

    The photocell is connected to a 10k ressitor which in turn is connected
    to ground, forming a voltage divider. This volt. div. output is what the
    ble_edge device sends. The current value (ressitance) of the photocell can
    be inferred from the data above.

    '''
    vplus = 3.3
    volt_divider_r2 = 10e3

    photocell_volt_div_val = _adc_parse_read_val_to_voltage(
        photocell_raw_read_val
    )

    # Find r2 from the voltage divider standard formula
    return (volt_divider_r2 * (vplus - photocell_volt_div_val) /
            photocell_volt_div_val)


def sens_photocell_estimate_lux(photocell_raw_read_val: int) -> int:
    '''
    Estimates the amount of lux :param photocell_raw_read_val represent.

    '''
    # The relation resistance-lux is given as a logarithmic scale that
    # highlights a few values, represented in the table below.
    # See Adafruit 161 photocell datasheet, section 'Measuring Light'.
    photocell_resistance_lux_estimation_table = {
        10e6:   0.1,
        1e6:    1,
        100e3:  10,
        10e3:   100,
        1e3:    1000
    }

    photocell_resistance = _sens_photocell_estimate_resistance(
        photocell_raw_read_val
    )

    rounded_to_nearest = min(
        list(photocell_resistance_lux_estimation_table.keys()),
        key=lambda x: abs(x - photocell_resistance)
    )

    return photocell_resistance_lux_estimation_table[rounded_to_nearest]


def sens_ir_infrared_source_detected(ir_raw_read_val: int) -> bool:
    '''
    Retrieve whether or not the IR sensor raw read val. indicates IR radiance
    detected.

    '''
    # The IR sensor goes low when it detects IR radiance, high otherwise.
    ir_sensor_max_output_voltage_low = 100e-3

    ir_sensor_output_volt = _adc_parse_read_val_to_voltage(ir_raw_read_val)

    return ir_sensor_output_volt > ir_sensor_max_output_voltage_low


def sens_hall_effect_get_magnetic_pole(hall_eff_raw_read_val: int) -> str:
    hall_effect_sensor_quiescent_voltage_interval = [0.9, 1.15]

    hall_eff_sens_output_volt = _adc_parse_read_val_to_voltage(
        hall_eff_raw_read_val
    )

    print('-----------------------> ', hall_eff_sens_output_volt)

    if (hall_eff_sens_output_volt >
        hall_effect_sensor_quiescent_voltage_interval[0]):
        return 'pole_south'

    if (hall_eff_sens_output_volt <
        hall_effect_sensor_quiescent_voltage_interval[1]):
        return 'pole_north'

    return 'no_magnetic_field'


def _sens_temp_denormalize(temp_sens_vout: int) -> int:
    '''
    The temp. sensor raw read val is normalized. This function de-normalizes the
    read value so it can be parsed to temperature later.

    '''
    # The temp. sensor raw value is normalized: it subtracted by 1646 mV and
    # multiplied by 2. See project-sensors excel spreadsheet in Drive
    subtactor_mv = 1646
    multiplicator = 2
    return temp_sens_vout / multiplicator + subtactor_mv


def sens_temp_estimate_temp(temp_raw_read_val: int) -> float:
    temp_sens_vout = _adc_parse_read_val_to_voltage(temp_raw_read_val)
    temp_sens_vout_mv = temp_sens_vout * 1e3

    temp_sens_vout_denormalized_mv = _sens_temp_denormalize(temp_sens_vout_mv)
    temp_sens_vout_denormalized = temp_sens_vout_denormalized_mv * 1e-3

    # The temp. sensor has an almost linear voltage-temperature relation. Thus,
    # it responds to a line equation, i.e. y = m*x + b. Using its max. and min.
    # temperature and voltage values as points of reference, calculate m and b
    # and apply the equation above to find the voltage.
    #
    # Notice that Y is the voltage and X is the temp., so the equation needs to
    # got from y = m*x + b to x = (y-b)/m
    min_temp = -50
    min_temp_vout = 3277e-3

    max_temp = 150
    max_temp_vout = 538e-3

    m = (max_temp_vout - min_temp_vout) / (max_temp - min_temp)
    b = max_temp_vout - m * max_temp

    return (temp_sens_vout_denormalized - b) / m


def sens_get_human_readable_value(sensor_name: str, raw_read_val: int) -> str:
    if sensor_name == 'SENSOR_MAGNETIC_FIELD':
        return str(sens_hall_effect_get_magnetic_pole(raw_read_val))
    elif sensor_name == 'SENSOR_PHOTOCELL':
        return str(sens_photocell_estimate_lux(raw_read_val))
    elif sensor_name == 'SENSOR_TEMP_DETECTOR':
        return str(sens_temp_estimate_temp(raw_read_val))
    elif sensor_name == 'SENSOR_IR_DETECTOR':
        return str(sens_ir_infrared_source_detected(raw_read_val))
    else:
        raise ValueError('unknown sensor')


if __name__ == '__main__':
    # SENSOR_MAGNETIC_FIELD                   142       pole_north
    # SENSOR_MAGNETIC_FIELD                   1034      pole_north
    # SENSOR_MAGNETIC_FIELD                   1550      pole_south

    # SENSOR_PHOTOCELL                        2396      100
    # SENSOR_TEMP_DETECTOR                    2097      -165806.16592243098
    # SENSOR_IR_DETECTOR                      2641      True

    print(sens_get_human_readable_value('SENSOR_MAGNETIC_FIELD', 142))
    print(sens_get_human_readable_value('SENSOR_MAGNETIC_FIELD', 1034))
    print(sens_get_human_readable_value('SENSOR_MAGNETIC_FIELD', 1550))