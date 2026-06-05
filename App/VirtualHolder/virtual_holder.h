/**
 * @file    virtual_holder.h
 * @brief   Virtual Holder: hotel room presence state machine.
 *
 * Inputs:
 *   - Door magnetic sensor (PB0): 0 = closed, 1 = open
 *   - PIR motion sensor  (PB1): 0 = no motion, 1 = motion detected
 *
 * States (vh_room_state_t):
 *   VACANT           — Room empty, no activity
 *   DOOR_OPEN        — Door opened, awaiting entry sequence
 *   GUEST_PRESENT    — Guest identified and present in room
 *   HOUSEKEEPING     — Housekeeping staff identified
 *   MAINTENANCE      — Maintenance staff identified
 *   UNEXPECTED       — Motion detected without valid door sequence (security)
 *
 * Profiles are differentiated by interaction patterns:
 *   - Guest: door opens → motion detected quickly → long occupancy
 *   - Housekeeping: door opens → motion detected → short stay → no overnight
 *   - Maintenance: door opens → limited motion (stays near door/panel) → short stay
 */

#ifndef VIRTUAL_HOLDER_H
#define VIRTUAL_HOLDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "shared_types.h"
#include "project_config.h"
#include "task_priorities.h"
#include "FreeRTOS.h"
#include "queue.h"

/* ------------------------------------------------------------------ */
/*  Timing Constants (ms)                                               */
/* ------------------------------------------------------------------ */
#define VH_POLL_PERIOD_MS           100U    /* Poll inputs every 100 ms */
#define VH_DOOR_DEBOUNCE_CYCLES      3U     /* 3 cycles = 300 ms debounce */
#define VH_ENTRY_TIMEOUT_MS       10000U    /* 10 s: max time door open → motion */
#define VH_PIR_ABSENCE_TIMEOUT_MS 300000U   /* 5 min: PIR silence → vacant */
#define VH_DOOR_AJAR_ALARM_MS     60000U    /* 1 min: door left open alarm */
#define VH_SHORT_STAY_THRESHOLD_MS 300000U  /* 5 min: short stay = housekeeping */
#define VH_MAINTENANCE_INACTIVITY_MS 60000U /* 1 min: maintenance PIR inactivity threshold */

/* ------------------------------------------------------------------ */
/*  Profile Detection Context                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t entry_time_ms;      /* Time from door open to first PIR */
    uint32_t stay_duration_ms;   /* Total stay duration */
    uint8_t  motion_event_count; /* Number of PIR triggers during stay */
    uint8_t  door_cycles;        /* Number of door open/close cycles */
} vh_profile_context_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the Virtual Holder state machine.
 */
void vh_init(void);

/**
 * @brief  VirtualHolderTask entry point.
 */
void Task_VirtualHolder(void *pvParameters);

/**
 * @brief  Get the current room state.
 */
vh_room_state_t vh_get_state(void);

/**
 * @brief  Get the detected profile.
 */
uint8_t vh_get_profile(void);

/**
 * @brief  Query whether an "unexpected presence" alarm is active.
 */
bool vh_is_alarm_active(void);

/**
 * @brief  Get the runtime state structure for diagnostics.
 */
const vh_state_t *vh_get_runtime_state(void);

#ifdef __cplusplus
}
#endif

#endif /* VIRTUAL_HOLDER_H */
