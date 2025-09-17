// Minimal Sync (PTP/SyncE/GNSS) mock for x86 simulator

#include <string.h>
#include "MplaneSync.h"

extern int mock_hal_get_sync_locked(void); // from mock_hal_control.c

halmplane_error_t halmplane_set_ptp_config(const ptp_config_t ptp_config) {
  (void)ptp_config;
  halmplane_error_t status = UNAVAILABLE;
  return status;
}

halmplane_error_t halmplane_get_ptp_status(ptp_status_t* ptp_status) {
  halmplane_error_t status = UNAVAILABLE;
  if (!ptp_status) return status;

  memset(ptp_status, 0, sizeof(*ptp_status));
  ptp_status->lock_state = (typeof(ptp_status->lock_state))(mock_hal_get_sync_locked() ? 0 : 1);
  ptp_status->clock_class = 248; // default holdover
  status = NONE;
  return status;
}

halmplane_error_t halmplane_set_synce_config(const synce_config_t synce_config) {
  halmplane_error_t status = NONE;
  (void)synce_config; 
  return status;
}

halmplane_error_t halmplane_get_synce_status(synce_status_t* synce_status) {
  halmplane_error_t status = UNAVAILABLE;
  if (!synce_status) return status;
  memset(synce_status, 0, sizeof(*synce_status));
  synce_status->sources.state = (typeof(synce_status->sources.state))(mock_hal_get_sync_locked() ? 0 : 1);
  status = NONE;
  return status;
}

halmplane_error_t halmplane_set_gnss_config(const gnss_config_t gnss_config) {
  halmplane_error_t status = NONE;
  (void)gnss_config; 
  return status;
}

halmplane_error_t halmplane_get_gnss_status(gnss_status_t* gnss_status) {
  halmplane_error_t status = UNAVAILABLE;
  if (!gnss_status) return status;
  memset(gnss_status, 0, sizeof(*gnss_status));
  gnss_status->gnss_sync_status = (typeof(gnss_status->gnss_sync_status))(mock_hal_get_sync_locked() ? 0 : 1);
  status = NONE;
  return status;
}

