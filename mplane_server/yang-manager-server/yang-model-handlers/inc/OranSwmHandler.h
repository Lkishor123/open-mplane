#pragma once

#include <memory>
#include <string>
#include "YangHandlerSysrepo.h"

#ifdef HAL_TEST
#include "mock_hal_control.h"
#endif

namespace Mplane {

class CallbackORanSoftwareMgr;  // Forward declaration

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

  //=== RPC handler methods ===
  bool rpcDownload(
      std::shared_ptr<sysrepo::Session> session,
      const std::string& rpcXpath,
      std::shared_ptr<YangParams> callList,
      std::shared_ptr<YangParams> retList);

  bool rpcInstall(
      std::shared_ptr<sysrepo::Session> session,
      const std::string& rpcXpath,
      std::shared_ptr<YangParams> callList,
      std::shared_ptr<YangParams> retList);

  bool rpcActivate(
      std::shared_ptr<sysrepo::Session> session,
      const std::string& rpcXpath,
      std::shared_ptr<YangParams> callList,
      std::shared_ptr<YangParams> retList);

  // Helper methods
  void statusOk(const std::string& rpc, std::shared_ptr<YangParams> retList);
  void statusFail(const std::string& rpc, std::shared_ptr<YangParams> retList,
                  const std::string& reason, const std::string& status = "FAILED");

  //=== Callback for software-inventory operational data ===
  std::shared_ptr<CallbackORanSoftwareMgr> mCallback;

  //=== Software download directory ===
  std::string mSoftwareDir;
};

} // namespace Mplane
