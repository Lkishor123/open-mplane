/*
 * x86 simulator init/exit hooks
 */

#include "libtinyxml2/tinyxml2.h"
#include "HalMplane.h"
#include "mock_hal_control.h"

int halmplane_init(tinyxml2::XMLDocument* doc) {
  (void)doc;
  mock_hal_reset();
  return 0;
}

int halmplane_exit() {
  mock_hal_reset();
  return 0;
}

