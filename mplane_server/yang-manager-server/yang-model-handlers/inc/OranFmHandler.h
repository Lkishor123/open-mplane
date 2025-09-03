/*
 * OranFmHandler: bridges HAL FM callbacks to o-ran-fm notifications
 */

#pragma once

#include <memory>
#include <string>

#include "YangHandlerSysrepo.h"
#include "MplaneAlarms.h"

namespace Mplane {

class OranFmHandler : public YangHandlerSysrepo {
 public:
  explicit OranFmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr);
  ~OranFmHandler() override;

  bool initialise() override;

 private:
  static void alarmCallbackWrapper(const halmplane_oran_alarm_t* alarm, void* store);
  void handleAlarm(const halmplane_oran_alarm_t* alarm);
};

} // namespace Mplane

