#pragma once

#include <memory>
#include <string>
#include "YangHandlerSysrepo.h"

#ifdef HAL_TEST
#include "mock_hal_control.h"
#endif

namespace Mplane {

class OranSwmHandler : public YangHandlerSysrepo {
 public:
  explicit OranSwmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr);
  ~OranSwmHandler() override;

  bool initialise() override;

 private:
#ifdef HAL_TEST
  static void swCallbackWrapper(const mock_sw_event_t* ev);
  void handleSwEvent(const mock_sw_event_t* ev);
#endif
  static const char* normalize_status(const char* res);
};

} // namespace Mplane
