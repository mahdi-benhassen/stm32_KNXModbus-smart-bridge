/**
 * @file    pi_controller.c
 * @brief   PI controller implementation with anti-windup clamping.
 *
 * The controller uses TickType_t (ms resolution via FreeRTOS tick) as its
 * time base. For higher precision, a hardware timer capture would be used.
 */

#include "pi_controller.h"
#include "FreeRTOS.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Module Variables                                                     */
/* ------------------------------------------------------------------ */
static pi_state_t g_pi_instances[PI_MAX_INSTANCES];

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void pi_controller_init(void)
{
    (void)memset(g_pi_instances, 0, sizeof(g_pi_instances));

    for (uint16_t i = 0U; i < PI_MAX_INSTANCES; i++) {
        g_pi_instances[i].kp      = PI_CONTROLLER_DEFAULT_KP;
        g_pi_instances[i].ki      = PI_CONTROLLER_DEFAULT_KI;
        g_pi_instances[i].out_min = PI_CONTROLLER_OUT_MIN;
        g_pi_instances[i].out_max = PI_CONTROLLER_OUT_MAX;
        g_pi_instances[i].active  = 0U;
    }
}

float pi_controller_evaluate(uint16_t block_id, float measured_val, float setpoint)
{
    if (block_id >= PI_MAX_INSTANCES) {
        return 0.0f;
    }

    pi_state_t *pi = &g_pi_instances[block_id];

    if (pi->active == 0U) {
        /* First call: configure and initialise */
        pi->active            = 1U;
        pi->setpoint          = setpoint;
        pi->integral          = 0.0f;
        pi->prev_error        = 0.0f;
        pi->last_sample_tick  = xTaskGetTickCount();
        pi->kp = PI_CONTROLLER_DEFAULT_KP;
        pi->ki = PI_CONTROLLER_DEFAULT_KI;
        pi->out_min = PI_CONTROLLER_OUT_MIN;
        pi->out_max = PI_CONTROLLER_OUT_MAX;
    }

    /* Compute time delta in seconds (FreeRTOS tick = 1 ms typical) */
    uint32_t now          = xTaskGetTickCount();
    uint32_t dt_ticks     = (now > pi->last_sample_tick) ? (now - pi->last_sample_tick) : 1U;
    float    dt           = (float)dt_ticks / 1000.0f;  /* seconds */
    pi->last_sample_tick  = now;

    /* Cap dt to prevent integral wind-up after a long pause */
    if (dt > 5.0f) {
        dt = 5.0f;
    }
    if (dt < 0.001f) {
        dt = 0.001f;  /* Minimum step to avoid division issues */
    }

    /* Compute error */
    float error = setpoint - measured_val;

    /* Proportional term */
    float p_term = pi->kp * error;

    /* Integral term with anti-windup */
    pi->integral += error * dt;
    float i_term = pi->ki * pi->integral;

    /* Clamp integral term to prevent wind-up */
    if (i_term > pi->out_max) {
        pi->integral = pi->out_max / pi->ki;
        i_term = pi->out_max;
    } else if (i_term < pi->out_min) {
        pi->integral = pi->out_min / pi->ki;
        i_term = pi->out_min;
    }

    /* Combine */
    float output = p_term + i_term;

    /* Output clamping */
    if (output > pi->out_max) {
        output = pi->out_max;
    }
    if (output < pi->out_min) {
        output = pi->out_min;
    }

    pi->prev_error = error;

    return output;
}

const pi_state_t *pi_controller_get_state(uint16_t block_id)
{
    if (block_id >= PI_MAX_INSTANCES) {
        return NULL;
    }
    return &g_pi_instances[block_id];
}

void pi_controller_reset(uint16_t block_id)
{
    if (block_id >= PI_MAX_INSTANCES) {
        return;
    }
    g_pi_instances[block_id].integral   = 0.0f;
    g_pi_instances[block_id].prev_error = 0.0f;
    g_pi_instances[block_id].active     = 0U;
}

void pi_controller_configure(uint16_t block_id, float kp, float ki,
                             float out_min, float out_max)
{
    if (block_id >= PI_MAX_INSTANCES) {
        return;
    }
    g_pi_instances[block_id].kp      = kp;
    g_pi_instances[block_id].ki      = ki;
    g_pi_instances[block_id].out_min = out_min;
    g_pi_instances[block_id].out_max = out_max;
}
