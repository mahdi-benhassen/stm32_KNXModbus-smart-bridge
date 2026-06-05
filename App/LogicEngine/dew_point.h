/**
 * @file    dew_point.h
 * @brief   Dew point calculation using the Magnus formula.
 *
 * Formula (Magnus-Tetens approximation, valid -40°C to +50°C):
 *
 *   alpha = (a * T) / (b + T) + ln(RH / 100)
 *   Td    = (b * alpha) / (a - alpha)
 *
 *   where:
 *     a  = 17.27  (Magnus constant over water)
 *     b  = 237.7  (Magnus constant over water)
 *     T  = air temperature in °C
 *     RH = relative humidity in %
 *     Td = dew point temperature in °C
 */

#ifndef DEW_POINT_H
#define DEW_POINT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Calculate the dew point temperature.
 *
 * @param temperature_c  Air temperature in degrees Celsius.
 * @param humidity_pct   Relative humidity in percent (0..100).
 *
 * @return Dew point temperature in degrees Celsius.
 *         Returns NAN on invalid input (RH <= 0, RH > 100, T < -40, T > 50).
 */
float dew_point_calculate(float temperature_c, float humidity_pct);

#ifdef __cplusplus
}
#endif

#endif /* DEW_POINT_H */
