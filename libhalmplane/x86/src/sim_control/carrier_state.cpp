#include "sim/carrier_state.h"
#include "MplaneUplaneConf.h"

#include <stdio.h>
#include <string.h>

// Internal state
static int g_sync_locked = 0;  // 0=UNLOCKED, 1=LOCKED
static halmplane_carrier_state_cb_t g_rx_carrier_cb = NULL;
static halmplane_carrier_state_cb_t g_tx_carrier_cb = NULL;

// Simple carrier state table
typedef struct {
    char name[64];
    carrier_state_t state;
    int in_use;
} carrier_entry_t;

#define MAX_CARRIERS 16
static carrier_entry_t g_rx_carriers[MAX_CARRIERS];
static carrier_entry_t g_tx_carriers[MAX_CARRIERS];

// Helper: find or create carrier entry
static carrier_entry_t* find_entry(carrier_entry_t* tbl, const char* name) {
    // Look for existing entry
    for (int i = 0; i < MAX_CARRIERS; ++i) {
        if (tbl[i].in_use && strncmp(tbl[i].name, name, sizeof(tbl[i].name)) == 0) {
            return &tbl[i];
        }
    }

    // Create new entry
    for (int i = 0; i < MAX_CARRIERS; ++i) {
        if (!tbl[i].in_use) {
            strncpy(tbl[i].name, name, sizeof(tbl[i].name) - 1);
            tbl[i].name[sizeof(tbl[i].name) - 1] = '\0';
            tbl[i].state = DISABLED;
            tbl[i].in_use = 1;
            return &tbl[i];
        }
    }

    return NULL;  // Table full
}

// Helper: parse state string to enum
static int parse_new_state(const char* new_state, carrier_state_t* out) {
    if (!new_state || !out) return 1;

    if (strcmp(new_state, "ACTIVE") == 0) {
        *out = READY;
        return 0;
    }
    if (strcmp(new_state, "INACTIVE") == 0) {
        *out = DISABLED;
        return 0;
    }
    if (strcmp(new_state, "SLEEP") == 0) {
        *out = BUSY;
        return 0;
    }

    return 1;  // Invalid state
}

// Public API implementation

void carrier_state_init(void) {
    carrier_state_reset();
}

void carrier_state_reset(void) {
    g_sync_locked = 0;
    g_rx_carrier_cb = NULL;
    g_tx_carrier_cb = NULL;
    memset(g_rx_carriers, 0, sizeof(g_rx_carriers));
    memset(g_tx_carriers, 0, sizeof(g_tx_carriers));
}

void carrier_state_set_rx_callback(halmplane_carrier_state_cb_t cb) {
    g_rx_carrier_cb = cb;
}

void carrier_state_set_tx_callback(halmplane_carrier_state_cb_t cb) {
    g_tx_carrier_cb = cb;
}

void carrier_state_set_sync_locked(bool locked) {
    g_sync_locked = locked ? 1 : 0;
}

bool carrier_state_get_sync_locked(void) {
    return g_sync_locked ? true : false;
}

int carrier_state_change_tx(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain,
    const char* new_state,
    int do_apply) {

    (void)chbw;
    (void)center;
    (void)gain;

    carrier_state_t target;
    if (parse_new_state(new_state, &target)) {
        return 1;  // Invalid state
    }

    // Enforce sync precondition for activation
    if (target == READY && !g_sync_locked) {
        return 1;  // Cannot activate without sync lock
    }

    fprintf(stderr, "[carrier_state] tx_carrier_state_change name=%s state=%s do_apply=%d\n",
            name, new_state, do_apply);
    fflush(stderr);

    if (!do_apply) {
        return 0;  // Validation only
    }

    carrier_entry_t* e = find_entry(g_tx_carriers, name);
    if (!e) {
        return 2;  // Table full
    }

    e->state = target;

    if (g_tx_carrier_cb) {
        g_tx_carrier_cb(name, target);
    }

    return 0;
}

int carrier_state_change_rx(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain_correction,
    const char* new_state,
    int do_apply) {

    (void)chbw;
    (void)center;
    (void)gain_correction;

    carrier_state_t target;
    if (parse_new_state(new_state, &target)) {
        return 1;  // Invalid state
    }

    // Enforce sync precondition for activation
    if (target == READY && !g_sync_locked) {
        return 1;  // Cannot activate without sync lock
    }

    if (!do_apply) {
        return 0;  // Validation only
    }

    carrier_entry_t* e = find_entry(g_rx_carriers, name);
    if (!e) {
        return 2;  // Table full
    }

    e->state = target;

    if (g_rx_carrier_cb) {
        g_rx_carrier_cb(name, target);
    }

    return 0;
}
