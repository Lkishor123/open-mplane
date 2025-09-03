// Minimal Sync (PTP/SyncE/GNSS) mock for x86 simulator

#include <string.h>
#include "MplaneSync.h"

extern int mock_hal_get_sync_locked(void); // from mock_hal_control.c

halmplane_error_t halmplane_set_ptp_config(const ptp_config_t ptp_config) {
  (void)ptp_config;
  return HALMPLANE_E_NONE;
}

halmplane_error_t halmplane_get_ptp_status(ptp_status_t* ptp_status) {
  if (!ptp_status) return HALMPLANE_E_FAILURE;
  memset(ptp_status, 0, sizeof(*ptp_status));
  ptp_status->lock_state = mock_hal_get_sync_locked() ? LOCKED : UNLOCKED;
  ptp_status->clock_class = 248; // default holdover
  return HALMPLANE_E_NONE;
}

halmplane_error_t halmplane_set_synce_config(const synce_config_t synce_config) {
  (void)synce_config; return HALMPLANE_E_NONE;
}

halmplane_error_t halmplane_get_synce_status(synce_status_t* synce_status) {
  if (!synce_status) return HALMPLANE_E_FAILURE;
  memset(synce_status, 0, sizeof(*synce_status));
  synce_status->sources.state = mock_hal_get_sync_locked() ? LOCKED : UNLOCKED;
  return HALMPLANE_E_NONE;
}

halmplane_error_t halmplane_set_gnss_config(const gnss_config_t gnss_config) {
  (void)gnss_config; return HALMPLANE_E_NONE;
}

halmplane_error_t halmplane_get_gnss_status(gnss_status_t* gnss_status) {
  if (!gnss_status) return HALMPLANE_E_FAILURE;
  memset(gnss_status, 0, sizeof(*gnss_status));
  gnss_status->gnss_sync_status = mock_hal_get_sync_locked() ? SYNCHRONIZED : ACQUIRING_SYNC;
  return HALMPLANE_E_NONE;
}

