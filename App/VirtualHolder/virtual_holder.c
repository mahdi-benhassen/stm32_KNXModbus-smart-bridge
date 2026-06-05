/**
 * @file    virtual_holder.c
 * @brief   Virtual Holder state machine implementation.
 *
 * State transition rules:
 *
 * ┌──────────┐   door open    ┌───────────┐
 * │  VACANT  │ ──────────────►│ DOOR_OPEN │
 * │          │◄────────────── │           │
 * └──────────┘ door close +   └─────┬─────┘
 *               no PIR              │ PIR trigger (quick: guest)
 *                  ▲                │ PIR trigger (slow: housekeeping)
 *                  │                ▼
 *          ┌───────┴───────┐  ┌──────────────┐
 *          │  any profile  │  │ GUEST_PRESENT│
 *          │  → departure  │  │ /HOUSEKEEPING│
 *          │  (door+PIR)   │  │ /MAINTENANCE │
 *          └───────────────┘  └──────────────┘
 *
 * ┌────────────┐   PIR w/o door   ┌────────────┐
 * │  VACANT    │ ────────────────►│ UNEXPECTED │
 * └────────────┘                   └────────────┘
 */

#include "virtual_holder.h"
#include "board.h"
#include "mapping_table.h"
#include "data_broker.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Module Variables                                                     */
/* ------------------------------------------------------------------ */
static vh_state_t           g_vh;
static vh_profile_context_t g_profile_ctx;
static bool                 g_alarm_active;

/* Debounce state */
static uint8_t  door_raw_history;
static uint8_t  door_filtered;       /* 0 = closed, 1 = open */
static uint8_t  door_prev_filtered;
static uint8_t  pir_raw_history;
static uint8_t  pir_filtered;        /* 0 = no motion, 1 = motion */
static uint8_t  pir_prev_filtered;

static uint8_t  door_edge_rising;    /* 0→1 transition detected */
static uint8_t  door_edge_falling;   /* 1→0 transition detected */
static uint8_t  pir_edge_rising;     /* 0→1 transition detected */

/* ------------------------------------------------------------------ */
/*  Debounce & Edge Detection                                           */
/* ------------------------------------------------------------------ */

static void vh_debounce_inputs(void)
{
    /* Shift histories and sample */
    door_raw_history = (uint8_t)((door_raw_history << 1U) & 0x07U);
    pir_raw_history  = (uint8_t)((pir_raw_history  << 1U) & 0x07U);

    if (READ_DOOR_SENSOR()) {
        door_raw_history |= 0x01U;
    }
    if (READ_PIR_SENSOR()) {
        pir_raw_history |= 0x01U;
    }

    /* Door debounce: all 3 samples must agree */
    if (door_raw_history == 0x07U) {
        door_filtered = 1U;
    } else if (door_raw_history == 0x00U) {
        door_filtered = 0U;
    }
    /* Else: hold previous value during bounce */

    /* PIR debounce: simpler, use majority */
    uint8_t ones = 0U;
    ones += (pir_raw_history & 0x01U) ? 1U : 0U;
    ones += (pir_raw_history & 0x02U) ? 1U : 0U;
    ones += (pir_raw_history & 0x04U) ? 1U : 0U;
    pir_filtered = (ones >= 2U) ? 1U : 0U;

    /* Edge detection */
    door_edge_rising  = (door_prev_filtered == 0U && door_filtered == 1U) ? 1U : 0U;
    door_edge_falling = (door_prev_filtered == 1U && door_filtered == 0U) ? 1U : 0U;
    pir_edge_rising   = (pir_prev_filtered  == 0U && pir_filtered  == 1U) ? 1U : 0U;

    door_prev_filtered = door_filtered;
    pir_prev_filtered  = pir_filtered;
}

/* ------------------------------------------------------------------ */
/*  State Transition Logic (deterministic switch-case)                  */
/* ------------------------------------------------------------------ */

/**
 * @brief  Evaluate state transitions. Called once per poll cycle.
 *
 * @return true if state changed, false otherwise.
 */
static bool vh_evaluate_transitions(void)
{
    vh_room_state_t prev_state = g_vh.current_state;
    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    g_vh.previous_state = prev_state;  /* Save for broadcast */

    switch (g_vh.current_state) {

    /* ------------------------------------------------------------------ */
    case VH_STATE_VACANT:
        if (door_edge_rising) {
            /* Door opened: start entry sequence */
            g_vh.current_state    = VH_STATE_DOOR_OPEN;
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open    = 1U;
            g_vh.pir_triggered   = 0U;
            g_profile_ctx.door_cycles++;
            g_alarm_active = false;
        } else if (pir_edge_rising) {
            /* PIR triggered without door: UNEXPECTED PRESENCE */
            g_vh.current_state       = VH_STATE_UNEXPECTED;
            g_vh.pir_triggered       = 1U;
            g_vh.unexpected_flag_cnt++;
            g_alarm_active           = true;
        } else if (door_filtered && !door_edge_rising) {
            /* Door became open without a detected rising edge (power-up) */
            g_vh.current_state    = VH_STATE_DOOR_OPEN;
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open     = 1U;
        }
        break;

    /* ------------------------------------------------------------------ */
    case VH_STATE_DOOR_OPEN:
        /* Check: door ajar too long */
        if (door_filtered &&
            ((now_ms - g_vh.door_open_timestamp) > VH_DOOR_AJAR_ALARM_MS)) {
            /* Door left open alarm: could be security issue */
            g_alarm_active = true;
        }

        if (pir_edge_rising) {
            /* Motion detected: someone entered */
            uint32_t entry_delay = now_ms - g_vh.door_open_timestamp;
            g_profile_ctx.entry_time_ms = entry_delay;

            g_vh.pir_triggered = 1U;
            g_vh.profile       = VH_PROFILE_GUEST;  /* Tentative */

            /* Profile heuristics */
            if (entry_delay < 3000U) {
                /* Quick entry after door opens: typical guest */
                g_vh.profile = VH_PROFILE_GUEST;
            } else if (entry_delay < 8000U) {
                /* Moderate delay: housekeeping (unlocking, gathering supplies) */
                g_vh.profile = VH_PROFILE_HOUSEKEEPING;
            } else {
                /* Long delay: possibly maintenance (checking equipment at door) */
                g_vh.profile = VH_PROFILE_MAINTENANCE;
            }

            /* Transition to occupancy */
            switch (g_vh.profile) {
            case VH_PROFILE_GUEST:
                g_vh.current_state = VH_STATE_GUEST_PRESENT;
                break;
            case VH_PROFILE_HOUSEKEEPING:
                g_vh.current_state = VH_STATE_HOUSEKEEPING;
                break;
            case VH_PROFILE_MAINTENANCE:
                g_vh.current_state = VH_STATE_MAINTENANCE;
                break;
            default:
                g_vh.current_state = VH_STATE_GUEST_PRESENT;
                break;
            }

            g_profile_ctx.motion_event_count = 1U;
            g_vh.state_entry_timestamp = now_ms;
        } else if (door_edge_falling) {
            /* Door closed without PIR: vacant */
            g_vh.current_state = VH_STATE_VACANT;
            g_vh.door_is_open  = 0U;
            g_alarm_active     = false;
        } else if (!door_filtered) {
            /* Door somehow became closed without edge detection */
            g_vh.current_state = VH_STATE_VACANT;
            g_vh.door_is_open  = 0U;
        } else {
            /* Door still open, no PIR yet: check timeout */
            if ((now_ms - g_vh.door_open_timestamp) > VH_ENTRY_TIMEOUT_MS) {
                /* Door open too long without entry: revert to vacant (door sensor may be faulty) */
                g_vh.current_state = VH_STATE_VACANT;
                g_vh.door_is_open  = 0U;
            }
        }
        break;

    /* ------------------------------------------------------------------ */
    case VH_STATE_GUEST_PRESENT:
        if (pir_edge_rising) {
            g_profile_ctx.motion_event_count++;
            g_vh.last_pir_timestamp = now_ms;
        }

        if (pir_filtered) {
            /* PIR active: reset absence timer */
            g_vh.last_pir_timestamp = now_ms;
        } else {
            /* Check PIR absence timeout */
            if ((now_ms - g_vh.last_pir_timestamp) > VH_PIR_ABSENCE_TIMEOUT_MS) {
                /* Guest likely left without using door sensor */
                g_vh.current_state = VH_STATE_VACANT;
                g_profile_ctx.stay_duration_ms = now_ms - g_vh.state_entry_timestamp;
            }
        }

        if (door_edge_rising) {
            /* Door opened while occupied: departure in progress */
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open = 1U;
        }

        if (door_edge_falling) {
            g_vh.door_is_open = 0U;
            /* Door closed during presence: could be someone leaving or entering.
             * Check PIR shortly after for re-entry. */
        }
        break;

    /* ------------------------------------------------------------------ */
    case VH_STATE_HOUSEKEEPING:
        if (pir_edge_rising) {
            g_profile_ctx.motion_event_count++;
            g_vh.last_pir_timestamp = now_ms;
        }

        if (pir_filtered) {
            g_vh.last_pir_timestamp = now_ms;
        } else {
            /* Housekeeping PIR absence typically means they left */
            if ((now_ms - g_vh.last_pir_timestamp) > VH_SHORT_STAY_THRESHOLD_MS) {
                g_vh.current_state = VH_STATE_VACANT;
                g_profile_ctx.stay_duration_ms = now_ms - g_vh.state_entry_timestamp;
            }
        }

        if (door_edge_rising) {
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open = 1U;
        }

        if (door_edge_falling) {
            g_vh.door_is_open = 0U;
            /* Door closing may indicate departure; if PIR inactive → vacant */
        }
        break;

    /* ------------------------------------------------------------------ */
    case VH_STATE_MAINTENANCE:
        if (pir_edge_rising) {
            g_profile_ctx.motion_event_count++;
            g_vh.last_pir_timestamp = now_ms;
        }

        if (pir_filtered) {
            g_vh.last_pir_timestamp = now_ms;
        } else {
            /* Maintenance may be stationary near panel */
            if ((now_ms - g_vh.last_pir_timestamp) > VH_MAINTENANCE_INACTIVITY_MS) {
                g_vh.current_state = VH_STATE_VACANT;
                g_profile_ctx.stay_duration_ms = now_ms - g_vh.state_entry_timestamp;
            }
        }

        if (door_edge_rising || door_edge_falling) {
            /* Door interaction during maintenance: update timers */
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open = door_filtered;
        }
        break;

    /* ------------------------------------------------------------------ */
    case VH_STATE_UNEXPECTED:
        g_alarm_active = true;

        if (door_edge_rising) {
            /* Door opening during unexpected: possibly resolves alarm */
            g_vh.current_state = VH_STATE_DOOR_OPEN;
            g_vh.door_open_timestamp = now_ms;
            g_vh.door_is_open = 1U;
            g_vh.pir_triggered = 0U;
            g_alarm_active = false;
        } else if (!pir_filtered) {
            /* PIR went silent: revert to vacant after grace period */
            uint32_t silence = now_ms - g_vh.last_pir_timestamp;
            if (silence > VH_PIR_ABSENCE_TIMEOUT_MS) {
                g_vh.current_state = VH_STATE_VACANT;
                g_alarm_active = false;
            }
        }
        break;

    default:
        /* Unknown state: reset to vacant */
        g_vh.current_state = VH_STATE_VACANT;
        break;
    }

    /* Update door/PIR runtime flags */
    g_vh.door_is_open  = door_filtered;
    g_vh.pir_triggered = pir_filtered;

    /* Track profile context */
    if (g_vh.current_state == VH_STATE_VACANT && prev_state != VH_STATE_VACANT) {
        /* Departure detected: compute stay duration */
        g_profile_ctx.stay_duration_ms = now_ms - g_vh.state_entry_timestamp;
    }

    return (g_vh.current_state != prev_state);
}

/* ------------------------------------------------------------------ */
/*  State Broadcast (to KNX / Modbus)                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief  Broadcast state/profile changes to the bus(ses).
 *         Uses dedicated KNX group objects for room state and profile.
 */
static void vh_broadcast_state(void)
{
    /* In production, this writes to specific KNX group objects
     * (e.g., G.O. 50 = Room State, G.O. 51 = Profile).
     * For now, post an event to the logic engine. */

    logic_event_item_t event;
    event.channel_id = 0xFFFFU;  /* Virtual holder source */
    event.source     = 3U;       /* VirtualHolder source */
    event.event_type = 2U;       /* profile change */
    (void)xQueueSend(qLogicEvent, &event, 0);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void vh_init(void)
{
    (void)memset(&g_vh,         0, sizeof(g_vh));
    (void)memset(&g_profile_ctx, 0, sizeof(g_profile_ctx));

    g_vh.current_state      = VH_STATE_VACANT;
    g_vh.previous_state     = VH_STATE_VACANT;
    g_vh.profile            = VH_PROFILE_UNKNOWN;

    door_raw_history   = 0U;
    door_filtered      = 0U;
    door_prev_filtered = 0U;
    pir_raw_history    = 0U;
    pir_filtered       = 0U;
    pir_prev_filtered  = 0U;
    g_alarm_active     = false;

    /* Initial input sample */
    vh_debounce_inputs();
}

void Task_VirtualHolder(void *pvParameters)
{
    (void)pvParameters;

    vh_init();

    for (;;) {
        /* Poll inputs with debounce */
        vh_debounce_inputs();

        /* Evaluate state transitions */
        bool state_changed = vh_evaluate_transitions();

        if (state_changed) {
            vh_broadcast_state();
        }

        /* Periodically broadcast state even if unchanged (heartbeat) */
        /* (optional: every 30 s) */

        vTaskDelay(pdMS_TO_TICKS(VH_POLL_PERIOD_MS));
    }
}

vh_room_state_t vh_get_state(void)
{
    return g_vh.current_state;
}

uint8_t vh_get_profile(void)
{
    return g_vh.profile;
}

bool vh_is_alarm_active(void)
{
    return g_alarm_active;
}

const vh_state_t *vh_get_runtime_state(void)
{
    return &g_vh;
}
