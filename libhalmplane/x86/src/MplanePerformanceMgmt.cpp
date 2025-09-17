// // Minimal Performance Management mock for x86 simulator

// #include <string.h>
// #include "MplanePerformanceMgmt.h"

// static halmplane_transceiver_meas_cb_t g_txrx_cb = 0;
// static halmplane_rx_window_meas_cb_t g_rxwin_cb = 0;
// static halmplane_tx_stats_meas_cb_t g_tx_cb = 0;
// static halmplane_epe_meas_cb_t g_epe_cb = 0;

// int halmplane_configPerfMeasurementParams(performance_measurement_params_t* config) {
//   (void)config; return 0;
// }

// int halmplane_activateTransceiverMeasObjects(transceiver_measurement_objects_t config,
//                                              halmplane_transceiver_meas_cb_t cb) {
//   (void)config; g_txrx_cb = cb; return 0;
// }

// int halmplane_activateRxWindowMeasObjects(rx_window_measurement_objects_t config,
//                                           halmplane_rx_window_meas_cb_t cb) {
//   (void)config; g_rxwin_cb = cb; return 0;
// }

// int halmplane_activateTxMeasObjects(const tx_measurement_objects_t config,
//                                     halmplane_tx_stats_meas_cb_t cb) {
//   (void)config; g_tx_cb = cb; return 0;
// }

// int halmplane_activateEpeMeasObjects(epe_measurement_objects_t config,
//                                      halmplane_epe_meas_cb_t cb) {
//   (void)config; g_epe_cb = cb; return 0;
// }

