/******************************************************************************
Finite State Machine
Project: GPS Logger
Description: The Finite State Machine that manage the GPS Logger

Generated by gv_fsm ruby gem, see https://rubygems.org/gems/gv_fsm
gv_fsm version 0.3.3
Generation date: 2022-07-20 19:18:11 +0200
Generated from: gps_logger.dot
The finite state machine has:
  6 states
  6 transition functions
******************************************************************************/

#include <syslog.h>
#include "fsm_gps_logger.h"

// SEARCH FOR Your Code Here FOR CODE INSERTION POINTS!

// GLOBALS
// State human-readable names
const char *state_names[] = {"uninitialized", "init", "idle", "run", "stop", "error"};

// List of state functions
state_func_t *const state_table[NUM_STATES] = {
    do_uninitialized,  // in state uninitialized
    do_init,           // in state init
    do_idle,           // in state idle
    do_run,            // in state run
    do_stop,           // in state stop
    do_error,          // in state error
};

// Table of transition functions
transition_func_t *const transition_table[NUM_STATES][NUM_STATES] = {
  	/* states:           uninitialized, init   , idle   , run    , stop   , error   */
  	/* uninitialized */ {NULL   , to_init, NULL   , NULL   , NULL   , NULL   }, 
  	/* init          */ {reset  , NULL   , to_idle, NULL   , NULL   , NULL   }, 
	/* idle          */ {reset  , to_init, stay   , to_run , NULL   , NULL   }, 
  	/* run           */ {NULL   , NULL   , NULL   , NULL   , to_stop, NULL   }, 
  	/* stop          */ {reset  , to_init, NULL   , NULL   , NULL   , NULL   }, 
  	/* error         */ {reset  , to_init, NULL   , NULL   , NULL   , NULL   }, 
};

/*  ____  _        _
 * / ___|| |_ __ _| |_ ___
 * \___ \| __/ _` | __/ _ \
 *  ___) | || (_| | ||  __/
 * |____/ \__\__,_|\__\___|
 *
 *   __                  _   _
 *  / _|_   _ _ __   ___| |_(_) ___  _ __  ___
 * | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */

// Function to be executed in state uninitialized
// valid return states: STATE_INIT
state_t do_uninitialized(state_data_t *data) {
    state_t next_state = STATE_INIT;

    syslog(LOG_INFO, "[FSM] In state uninitialized");
    /* Your Code Here */

    switch(next_state) {
        case STATE_INIT:
            break;
        default:
            syslog(LOG_WARNING,
                   "[FSM] Cannot pass from uninitialized to %s, remaining in "
                   "this state",
                   state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

// Function to be executed in state init
// valid return states: STATE_UNINITIALIZED, STATE_IDLE
state_t do_init(state_data_t *data) {
    state_t next_state = STATE_UNINITIALIZED;

    syslog(LOG_INFO, "[FSM] In state init");
    /* Your Code Here */

    switch(next_state) {
        case STATE_UNINITIALIZED:
        case STATE_IDLE:
            break;
        default:
            syslog(LOG_WARNING,
                   "[FSM] Cannot pass from init to %s, remaining in this state",
                   state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

// Function to be executed in state idle
// valid return states: NO_CHANGE, STATE_UNINITIALIZED, STATE_INIT, STATE_IDLE,
// STATE_RUN
state_t do_idle(state_data_t *data) {
    state_t next_state = NO_CHANGE;

    syslog(LOG_INFO, "[FSM] In state idle");
    /* Your Code Here */

    switch(next_state) {
        case NO_CHANGE:
        case STATE_UNINITIALIZED:
        case STATE_INIT:
        case STATE_IDLE:
        case STATE_RUN:
            break;
        default:
            syslog(LOG_WARNING,
                   "[FSM] Cannot pass from idle to %s, remaining in this state",
                   state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

// Function to be executed in state run
// valid return states: STATE_STOP
state_t do_run(state_data_t *data) {
    state_t next_state = STATE_STOP;

    syslog(LOG_INFO, "[FSM] In state run");
    /* Your Code Here */

    switch(next_state) {
        case STATE_STOP:
            break;
        default:
            syslog(LOG_WARNING,
                   "[FSM] Cannot pass from run to %s, remaining in this state",
                   state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

// Function to be executed in state stop
// valid return states: STATE_UNINITIALIZED, STATE_INIT
state_t do_stop(state_data_t *data) {
    state_t next_state = STATE_UNINITIALIZED;

    syslog(LOG_INFO, "[FSM] In state stop");
    /* Your Code Here */

    switch(next_state) {
        case STATE_UNINITIALIZED:
        case STATE_INIT:
            break;
        default:
            syslog(LOG_WARNING,
                   "[FSM] Cannot pass from stop to %s, remaining in this state",
                   state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

// Function to be executed in state error
// valid return states: STATE_UNINITIALIZED, STATE_INIT
state_t do_error(state_data_t *data) {
    state_t next_state = STATE_UNINITIALIZED;

    syslog(LOG_INFO, "[FSM] In state error");
    /* Your Code Here */

    switch(next_state) {
        case STATE_UNINITIALIZED:
        case STATE_INIT:
            break;
        default:
            syslog(
                LOG_WARNING,
                "[FSM] Cannot pass from error to %s, remaining in this state",
                state_names[next_state]);
            next_state = NO_CHANGE;
    }

    return next_state;
}

/*  _____                    _ _   _
 * |_   _| __ __ _ _ __  ___(_) |_(_) ___  _ __
 *   | || '__/ _` | '_ \/ __| | __| |/ _ \| '_ \
 *   | || | | (_| | | | \__ \ | |_| | (_) | | | |
 *   |_||_|  \__,_|_| |_|___/_|\__|_|\___/|_| |_|
 *
 *   __                  _   _
 *  / _|_   _ _ __   ___| |_(_) ___  _ __  ___
 * | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */

// This function is called in 4 transitions:
// 1. from uninitialized to init
// 2. from idle to init
// 3. from stop to init
// 4. from error to init
void to_init(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition to_init");
    /* Your Code Here */
}

// This function is called in 4 transitions:
// 1. from init to uninitialized
// 2. from idle to uninitialized
// 3. from stop to uninitialized
// 4. from error to uninitialized
void reset(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition reset");
    /* Your Code Here */
}

// This function is called in 1 transition:
// 1. from init to idle
void to_idle(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition to_idle");
    /* Your Code Here */
}

// This function is called in 1 transition:
// 1. from idle to idle
void stay(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition stay");
    /* Your Code Here */
}

// This function is called in 1 transition:
// 1. from idle to run
void to_run(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition to_run");
    /* Your Code Here */
}

// This function is called in 1 transition:
// 1. from run to stop
void to_stop(state_data_t *data) {
    syslog(LOG_INFO, "[FSM] State transition to_stop");
    /* Your Code Here */
}

/*  ____  _        _
 * / ___|| |_ __ _| |_ ___
 * \___ \| __/ _` | __/ _ \
 *  ___) | || (_| | ||  __/
 * |____/ \__\__,_|\__\___|
 *
 *
 *  _ __ ___   __ _ _ __   __ _  __ _  ___ _ __
 * | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '__|
 * | | | | | | (_| | | | | (_| | (_| |  __/ |
 * |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_|
 *                              |___/
 */

state_t run_state(state_t cur_state, state_data_t *data) {
    state_t new_state = state_table[cur_state](data);

    if(new_state == NO_CHANGE) {
		new_state = cur_state;
	}

    transition_func_t *transition = transition_table[cur_state][new_state];

    if(transition) {
		transition(data);
	}

    return new_state == NO_CHANGE ? cur_state : new_state;
};

#ifdef TEST_MAIN
#include <unistd.h>
int main() {
    state_t cur_state = STATE_UNINITIALIZED;
    openlog("SM", LOG_PID | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "Starting SM");
    do {
        cur_state = run_state(cur_state, NULL);
        sleep(1);
    } while(1);
    return 0;
}
#endif
