// Minimal mock implementation to support x86 simulator control and HAL API
// Refactored to delegate to modular components

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mock_hal_control.h"
#include "sim/alarm_ipc.h"
#include "sim/carrier_state.h"

// Software mgmt outcomes (textual status tracking)
static char g_sw_download_result[32] = "OK";
static char g_sw_install_result[32] = "OK";
static char g_sw_activate_result[32] = "OK";
static char g_sw_reset_result[32] = "OK";
static char g_sw_slot[64] = "";
static char g_sw_file[128] = "";

// SW callback
static mock_sw_cb_t g_sw_cb = NULL;

void mock_hal_reset(void) {
    // Reset all modules
    carrier_state_reset();
    alarm_ipc_cleanup();

    // Reset SW management state
    g_sw_cb = NULL;
    strncpy(g_sw_download_result, "OK", sizeof(g_sw_download_result) - 1);
    strncpy(g_sw_install_result, "OK", sizeof(g_sw_install_result) - 1);
    strncpy(g_sw_activate_result, "OK", sizeof(g_sw_activate_result) - 1);
    strncpy(g_sw_reset_result, "OK", sizeof(g_sw_reset_result) - 1);
    g_sw_slot[0] = '\0';
    g_sw_file[0] = '\0';
}

void mock_hal_set_sync_locked(bool locked) {
    carrier_state_set_sync_locked(locked);
}

bool mock_hal_get_sync_locked(void) {
    return carrier_state_get_sync_locked();
}

int mock_hal_inject_alarm(
    uint16_t fault_id,
    const char* fault_source,
    int severity,
    bool is_cleared,
    const char* fault_text) {

    return alarm_ipc_send_alarm(fault_id, fault_source, severity, is_cleared, fault_text);
}

int mock_hal_register_sw_cb(mock_sw_cb_t cb) {
    g_sw_cb = cb;
    return 0;
}

static void emit_sw_event(const char* phase, const char* result) {
    if (!g_sw_cb) return;

    mock_sw_event_t ev = {
        phase,
        result,
        g_sw_slot[0] ? g_sw_slot : NULL,
        g_sw_file[0] ? g_sw_file : NULL
    };
    g_sw_cb(&ev);
}

void mock_hal_set_sw_result(const char* phase, const char* result) {
    mock_hal_set_sw_result_ext(phase, result, NULL, NULL);
}

void mock_hal_set_sw_result_ext(
    const char* phase,
    const char* result,
    const char* slot,
    const char* file) {

    if (!phase || !result) return;

    if (slot) {
        strncpy(g_sw_slot, slot, sizeof(g_sw_slot) - 1);
        g_sw_slot[sizeof(g_sw_slot) - 1] = '\0';
    }
    if (file) {
        strncpy(g_sw_file, file, sizeof(g_sw_file) - 1);
        g_sw_file[sizeof(g_sw_file) - 1] = '\0';
    }

    if (strcmp(phase, "download") == 0) {
        strncpy(g_sw_download_result, result, sizeof(g_sw_download_result) - 1);
    } else if (strcmp(phase, "install") == 0) {
        strncpy(g_sw_install_result, result, sizeof(g_sw_install_result) - 1);
    } else if (strcmp(phase, "activate") == 0) {
        strncpy(g_sw_activate_result, result, sizeof(g_sw_activate_result) - 1);
    } else if (strcmp(phase, "reset") == 0) {
        strncpy(g_sw_reset_result, result, sizeof(g_sw_reset_result) - 1);
    }

    emit_sw_event(phase, result);
}

int mock_hal_status(char* buf, size_t buflen) {
    if (!buf || buflen == 0) return 1;

    int n = snprintf(buf, buflen,
        "{\"sync_locked\":%s,\"sw\":{\"download\":\"%s\",\"install\":\"%s\",\"activate\":\"%s\",\"reset\":\"%s\"}}",
        carrier_state_get_sync_locked() ? "true" : "false",
        g_sw_download_result,
        g_sw_install_result,
        g_sw_activate_result,
        g_sw_reset_result);

    return (n < 0 || (size_t)n >= buflen) ? 2 : 0;
}
