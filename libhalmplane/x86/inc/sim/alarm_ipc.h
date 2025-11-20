#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct halmplane_oran_alarm_s halmplane_oran_alarm_t;
typedef void (*halmplane_oran_alarm_cb_t)(const halmplane_oran_alarm_t*, void*);

/**
 * Initialize alarm IPC listener
 * Creates Unix socket and starts background thread
 * @return 0 on success, non-zero on error
 */
int alarm_ipc_init(void);

/**
 * Cleanup alarm IPC resources
 * Stops listener thread and removes socket file
 */
void alarm_ipc_cleanup(void);

/**
 * Register callback for receiving alarms
 * @param cb Callback function to invoke when alarm received
 */
void alarm_ipc_set_callback(halmplane_oran_alarm_cb_t cb);

/**
 * Send alarm via IPC (client function)
 * @param fault_id Alarm fault ID
 * @param fault_source Source of the fault
 * @param severity Severity level (0=CRITICAL, 1=MAJOR, 2=MINOR, 3=WARNING)
 * @param is_cleared True if alarm is cleared
 * @param fault_text Description text
 * @return 0 on success, non-zero on error
 */
int alarm_ipc_send_alarm(
    uint16_t fault_id,
    const char* fault_source,
    int severity,
    bool is_cleared,
    const char* fault_text);

/**
 * Check if listener is running
 * @return true if listener thread is active
 */
bool alarm_ipc_is_running(void);

#ifdef __cplusplus
}
#endif
