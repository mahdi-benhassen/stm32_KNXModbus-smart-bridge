/**
 * @file    pi_controller.h
 * @brief   Proportional-Integral (PI) controller for temperature and
 *          humidity regulation.
 *
 * Formula:
 *   e(t)  = setpoint - measured_value
 *   u(t)  = Kp * e(t) + Ki * integral(e(t)) dt
 *
 * Anti-windup: integral term is clamped to prevent saturation.
 * Output is clamped to [out_min, out_max].
 */

#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "shared_types.h"
#include "project_config.h"
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Public Types                                                        */
/* ------------------------------------------------------------------ */

/** Runtime pool of PI controller instances (one per logic block that uses PI) */
#define PI_MAX_INSTANCES  MAX_LOGIC_BLOCKS

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise all PI controller instances.
 */
void pi_controller_init(void);

/**
 * @brief  Evaluate a PI control iteration for a given logic block.
 *
 * @param block_id      Logic block index (used as instance identifier).
 * @param measured_val  The current process variable (e.g., temperature).
 * @param setpoint      The desired target value.
 *
 * @return  Controller output (manipulated variable), clamped to [out_min, out_max].
 */
float pi_controller_evaluate(uint16_t block_id, float measured_val, float setpoint);

/**
 * @brief  Get the PI controller instance state for diagnostics.
 *
 * @param block_id  Logic block index.
 * @return Pointer to pi_state_t, or NULL if block_id is invalid.
 */
const pi_state_t *pi_controller_get_state(uint16_t block_id);

/**
 * @brief  Reset the integrator for a given block (e.g., after a mode change).
 */
void pi_controller_reset(uint16_t block_id);

/**
 * @brief  Configure the PI parameters for a specific block at runtime.
 */
void pi_controller_configure(uint16_t block_id, float kp, float ki,
                             float out_min, float out_max);

#ifdef __cplusplus
}
#endif

#endif /* PI_CONTROLLER_H */
