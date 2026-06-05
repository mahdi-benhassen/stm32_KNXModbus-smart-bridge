/**
 * @file    dew_point.c
 * @brief   Dew point calculation using the Magnus approximation.
 *
 * Input validation enforces physical limits:
 *   - Temperature: -40.0°C .. +50.0°C
 *   - Humidity:     0.1% .. 100.0%
 *
 * Output is constrained to the same temperature range.
 */

#include "dew_point.h"
#include "project_config.h"
#include <math.h>

/* Magnus constants for water surface */
#define MAGNUS_A  17.27f
#define MAGNUS_B  237.7f

/* Input limits */
#define DP_TEMP_MIN  (-40.0f)
#define DP_TEMP_MAX   (50.0f)
#define DP_HUM_MIN     (0.1f)
#define DP_HUM_MAX   (100.0f)

float dew_point_calculate(float temperature_c, float humidity_pct)
{
    /* Input validation */
    if (temperature_c < DP_TEMP_MIN || temperature_c > DP_TEMP_MAX) {
        return NAN;
    }
    if (humidity_pct < DP_HUM_MIN || humidity_pct > DP_HUM_MAX) {
        return NAN;
    }
    if (!isfinite(temperature_c) || !isfinite(humidity_pct)) {
        return NAN;
    }

    /* Saturate at 100% — at RH=100%, dew point = air temperature */
    if (humidity_pct >= 100.0f) {
        return temperature_c;
    }

    float T  = temperature_c;
    float RH = humidity_pct;

    /* Magnus formula */
    float alpha = (MAGNUS_A * T) / (MAGNUS_B + T) + logf(RH / 100.0f);

    float Td = (MAGNUS_B * alpha) / (MAGNUS_A - alpha);

    /* Physical sanity: dew point cannot exceed air temperature */
    if (Td > T) {
        Td = T;
    }

    /* Clamp to valid range */
    if (Td < DP_TEMP_MIN) {
        Td = DP_TEMP_MIN;
    }
    if (Td > DP_TEMP_MAX) {
        Td = DP_TEMP_MAX;
    }

    return Td;
}
