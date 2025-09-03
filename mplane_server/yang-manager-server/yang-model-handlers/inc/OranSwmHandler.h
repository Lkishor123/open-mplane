/*
 * OranSwmHandler: publishes o-ran-software-management notifications and maintains inventory
 */

#pragma once

#include <memory>
#include <string>

#include "YangHandlerSysrepo.h"

// from mock HAL control (x86)
typedef struct mock_sw_event_s mock_sw_event_t;
typedef void (*mock_sw_cb_t)(const mock_sw_event_t* ev);
int mock_hal_register_sw_cb(mock_sw_cb_t cb);

namespace Mplane {

class OranSwmHandler : public YangHandlerSysrepo {
 public:
  explicit OranSwmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr);
  ~OranSwmHandler() override;

  bool initialise() override;

 private:
  static void swCallbackWrapper(const mock_sw_event_t* ev);
  void handleSwEvent(const mock_sw_event_t* ev);
  static const char* normalize_status(const char* res);
};

} // namespace Mplane

