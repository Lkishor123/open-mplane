/*
 * OranSwmHandler: publishes o-ran-software-management notifications and maintains inventory
 */

#include <map>
#include <string>

#include "OranSwmHandler.h"

using namespace Mplane;

static OranSwmHandler* g_oranSwmHandler = nullptr;

OranSwmHandler::OranSwmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr)
  : YangHandlerSysrepo("o-ran-software-management", moduleMgr, "OranSwmHandler") {}

OranSwmHandler::~OranSwmHandler() { if (g_oranSwmHandler == this) g_oranSwmHandler = nullptr; }

bool OranSwmHandler::initialise() {
  g_oranSwmHandler = this;
  mock_hal_register_sw_cb(&OranSwmHandler::swCallbackWrapper);
  return true;
}

void OranSwmHandler::swCallbackWrapper(const mock_sw_event_t* ev)
{
  if (g_oranSwmHandler) g_oranSwmHandler->handleSwEvent(ev);
}

const char* OranSwmHandler::normalize_status(const char* res)
{
  if (!res) return "FAILED";
  if (std::string(res) == "OK") return "COMPLETED";
  return res;
}

void OranSwmHandler::handleSwEvent(const mock_sw_event_t* ev)
{
  if (!ev || !ev->phase || !ev->result) return;
  std::string phase(ev->phase);
  std::string resultStr(normalize_status(ev->result));

  // Send notifications
  if (phase == "download") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("download-event");
    std::string base = notif + "/download-notification";
    if (ev->file) params->addParam(std::string(ev->file), base + "/file-name");
    params->addParam(resultStr, base + "/status");
    sendNotification(notif, params);
  } else if (phase == "install") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("install-event");
    std::string base = notif + "/install-notification";
    params->addParam(resultStr, base + "/status");
    if (ev->slot) params->addParam(std::string(ev->slot), base + "/slot-name");
    sendNotification(notif, params);

    // Update software-inventory based on result
    if (ev->slot) {
      std::string slot(ev->slot);
      std::string slotPath = path("software-inventory/software-slot[name='" + slot + "']");
      createListEntry(slotPath, std::map<std::string, std::string>{ {"name", slot} });
      if (resultStr == "COMPLETED")
        createItemStr(slotPath + "/status", "VALID");
      else if (resultStr == "INTEGRITY_ERROR")
        createItemStr(slotPath + "/status", "INVALID");
      else
        createItemStr(slotPath + "/status", "INVALID");
      // Not active/running yet
      createItemStr(slotPath + "/active", "false");
      createItemStr(slotPath + "/running", "false");
    }
  } else if (phase == "activate") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("activation-event");
    std::string base = notif + "/activation-notification";
    params->addParam(resultStr, base + "/status");
    if (ev->slot) params->addParam(std::string(ev->slot), base + "/slot-name");
    sendNotification(notif, params);

    // Mark slot active/running on success
    if (ev->slot) {
      std::string slot(ev->slot);
      std::string slotPath = path("software-inventory/software-slot[name='" + slot + "']");
      if (resultStr == "COMPLETED") {
        createItemStr(slotPath + "/active", "true");
        createItemStr(slotPath + "/running", "true");
      }
    }
  } else if (phase == "reset") {
    // No notification in SWM; the system reboot is out of scope for mock
  }
}

