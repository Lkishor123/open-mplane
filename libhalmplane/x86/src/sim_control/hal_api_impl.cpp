/*
 * HAL API Implementation for x86 Simulator
 * Delegates to internal modules (alarm_ipc, carrier_state)
 */

#include "MplaneAlarms.h"
#include "MplaneUplaneConf.h"
#include "MplaneSync.h"

#include "sim/alarm_ipc.h"
#include "sim/carrier_state.h"

// ===================== Alarm APIs =====================

int halmplane_registerOranAlarmCallback(halmplane_oran_alarm_cb_t cb) {
    alarm_ipc_set_callback(cb);

    // Start the socket listener if not already running
    if (!alarm_ipc_is_running()) {
        return alarm_ipc_init();
    }

    return 0;
}

// ===================== Carrier APIs =====================

int halmplane_register_rx_carrier_state_cb(halmplane_carrier_state_cb_t cb) {
    carrier_state_set_rx_callback(cb);
    return 0;
}

int halmplane_register_tx_carrier_state_cb(halmplane_carrier_state_cb_t cb) {
    carrier_state_set_tx_callback(cb);
    return 0;
}

int halmplane_tx_carrier_state_change(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain,
    const char* new_state,
    int do_apply) {

    return carrier_state_change_tx(name, chbw, center, gain, new_state, do_apply);
}

int halmplane_rx_carrier_state_change(
    const char* name,
    uint64_t chbw,
    uint64_t center,
    double gain_correction,
    const char* new_state,
    int do_apply) {

    return carrier_state_change_rx(name, chbw, center, gain_correction, new_state, do_apply);
}

// ===================== EAXC and Compression Stubs =====================

int halmplane_update_rx_eaxc(const char* endpoint_name, e_axcid_t* eaxc) {
    (void)endpoint_name;
    (void)eaxc;
    return 0;
}

int halmplane_update_tx_eaxc(const char* endpoint_name, e_axcid_t* eaxc) {
    (void)endpoint_name;
    (void)eaxc;
    return 0;
}

int halmplane_update_rx_endpoint_compression(
    const char* endpoint_name, compression_t* compression) {
    (void)endpoint_name;
    (void)compression;
    return 0;
}

int halmplane_update_tx_endpoint_compression(
    const char* endpoint_name, compression_t* compression) {
    (void)endpoint_name;
    (void)compression;
    return 0;
}

int halmplane_update_rx_endpoint_compression_dyn_config(
    const char* endpoint_name, dynamic_compression_configuration_t* config) {
    (void)endpoint_name;
    (void)config;
    return 0;
}

int halmplane_update_tx_endpoint_compression_dyn_config(
    const char* endpoint_name, dynamic_compression_configuration_t* config) {
    (void)endpoint_name;
    (void)config;
    return 0;
}

int halmplane_setUPlaneConfiguration(user_plane_configuration_t* cfg) {
    (void)cfg;
    return 0;
}
