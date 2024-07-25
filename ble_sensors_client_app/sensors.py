def _sens_photocell_estimate_resistance(photocell_read_val_mv_mv: int) -> int:
    '''
    Estimate the photocell current resistance value from its raw read value.

    The photocell is connected to a 10k ressitor which in turn is connected
    to ground, forming a voltage divider. This volt. div. output is what the
    ble_edge device sends. The current value (ressitance) of the photocell can
    be inferred from the data above.

    '''
    vplus = 3.3
    volt_divider_r2 = 10e3

    photocell_read_val_mv_v = photocell_read_val_mv_mv * 1e-3

    # Find r2 from the voltage divider standard formula
    return (volt_divider_r2 * (vplus - photocell_read_val_mv_v*1e-3) /
            photocell_read_val_mv_v)


def sens_photocell_light_detected(photocell_read_val_mv: int) -> int:
    '''
    Estimates the amount of lux :param photocell_read_val_mv represent.

    '''
    # Measurements:
    #   - with artificial light in the evening: ~14110
    #   - with artificial light in the evening putting finger: ~68740
    photocell_resistance = _sens_photocell_estimate_resistance(
        photocell_read_val_mv
    )

    # Set the threshold to 30 kOhms
    threshold = 30e3

    return photocell_resistance < threshold


def sens_ir_infrared_source_detected(ir_read_val_mv: int) -> bool:
    '''
    Retrieve whether or not the IR sensor raw read val. indicates IR radiance
    detected.

    '''
    # The IR sensor goes low when it detects IR radiance, high otherwise.
    # According to the datasheet, this is 100mV, but it reads actually ~140
    # in the presence of IR. Leave it as 500 in case.
    ir_sensor_max_output_voltage_low_mv = 500

    return ir_read_val_mv < ir_sensor_max_output_voltage_low_mv


def sens_hall_effect_get_magnetic_pole(hall_eff_read_val_mv: int) -> str:
    hall_effect_sensor_quiescent_voltage_interval = [0.9e3, 1.15e3] # mv

    if (hall_eff_read_val_mv >
        hall_effect_sensor_quiescent_voltage_interval[1]):
        return 'pole_south'

    if (hall_eff_read_val_mv <
        hall_effect_sensor_quiescent_voltage_interval[0]):
        return 'pole_north'

    return 'no_magnetic_field'


def _sens_temp_denormalize(temp_sens_vout_mv: int) -> int:
    '''
    The temp. sensor raw read val is normalized. This function de-normalizes the
    read value so it can be parsed to temperature later.

    '''
    # The temp. sensor raw value is normalized: it subtracted by 1646 mV and
    # multiplied by 2. See project-sensors excel spreadsheet in Drive
    subtactor_mv = 1646
    multiplicator = 2
    return temp_sens_vout_mv / multiplicator + subtactor_mv


def sens_temp_estimate_temp(temp_read_val_mv: int) -> float:
    temp_sens_vout_denormalized_mv = _sens_temp_denormalize(temp_read_val_mv)
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


def sens_get_human_readable_value(sensor_name: str, read_val_mv: int) -> str:
    if sensor_name == 'SENSOR_MAGNETIC_FIELD':
        return str(sens_hall_effect_get_magnetic_pole(read_val_mv))

    elif sensor_name == 'SENSOR_PHOTOCELL':
        return ('Light detected'
                if sens_photocell_light_detected(read_val_mv)
                else 'No light detected')

    elif sensor_name == 'SENSOR_TEMP_DETECTOR':
        return str(sens_temp_estimate_temp(read_val_mv))

    elif sensor_name == 'SENSOR_IR_DETECTOR':
        return ('IR detected'
                if sens_ir_infrared_source_detected(read_val_mv)
                else 'No IR detected')

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