#pragma once

#include <stdint.h>
#include <stdbool.h>

// Include MplaneUplaneConf.h to get carrier_state_t definition
#include "MplaneUplaneConf.h"

#ifdef __cplusplus
extern "C" {
#endif

// carrier_state_t is defined in MplaneUplaneConf.h as:
// typedef enum carrier_state_e {
//   DISABLED = 0,
//   BUSY = 1,
//   READY = 2,
// } carrier_state_t;

// Carrier state callback type
typedef void (*halmplane_carrier_state_cb_t)(const char* name, carrier_state_t state);

/**
 * Initialize carrier state module
 */
void carrier_state_init(void);

/**
 * Reset carrier state tables
 */
void carrier_state_reset(void);

/**
 * Register RX carrier state callback
 * @param cb Callback function
 */
void carrier_state_set_rx_callback(halmplane_carrier_state_cb_t cb);

/**
 * Register TX carrier state callback
 * @param cb Callback function
 */
void carrier_state_set_tx_callback(halmplane_carrier_state_cb_t cb);

/**
 * Set sync lock state (required for carrier activation)
 * @param locked True if sync is locked
 */
void carrier_state_set_sync_locked(bool locked);

/**
 * Get sync lock state
 * @return True if sync is locked
 */
bool carrier_state_get_sync_locked(void);

/**
 * Change TX carrier state
 * @param name Carrier name
 * @param chbw Channel bandwidth
 * @param center Center frequency
 * @param gain Gain value
 * @param new_state Target state ("ACTIVE", "INACTIVE", "SLEEP")
 * @param do_apply 1 to apply, 0 to validate only
 * @return 0 on success, non-zero on error
 */
int carrier_state_change_tx(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain,
    const char* new_state,
    int do_apply);

/**
 * Change RX carrier state
 * @param name Carrier name
 * @param chbw Channel bandwidth
 * @param center Center frequency
 * @param gain_correction Gain correction value
 * @param new_state Target state ("ACTIVE", "INACTIVE", "SLEEP")
 * @param do_apply 1 to apply, 0 to validate only
 * @return 0 on success, non-zero on error
 */
int carrier_state_change_rx(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain_correction,
    const char* new_state,
    int do_apply);

#ifdef __cplusplus
}
#endif
