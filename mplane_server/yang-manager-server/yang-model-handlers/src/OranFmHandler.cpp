/*
 * OranFmHandler: bridges HAL FM callbacks to o-ran-fm notifications
 */

#include <ctime>
#include <iomanip>
#include <sstream>

#include "OranFmHandler.h"

using namespace Mplane;

static OranFmHandler* g_oranFmHandler = nullptr;

static std::string iso8601_now()
{
  std::time_t t = std::time(nullptr);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

OranFmHandler::OranFmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr)
  : YangHandlerSysrepo("o-ran-fm", moduleMgr, "OranFmHandler") {}

OranFmHandler::~OranFmHandler() { if (g_oranFmHandler == this) g_oranFmHandler = nullptr; }

bool OranFmHandler::initialise() {
  g_oranFmHandler = this;
  halmplane_registerOranAlarmCallback(&OranFmHandler::alarmCallbackWrapper);
  return true;
}

void OranFmHandler::alarmCallbackWrapper(const halmplane_oran_alarm_t* alarm, void* store)
{
  (void)store;
  if (g_oranFmHandler) g_oranFmHandler->handleAlarm(alarm);
}

void OranFmHandler::handleAlarm(const halmplane_oran_alarm_t* alarm)
{
  // Build notification params
  std::shared_ptr<YangParams> params(std::make_shared<YangParams>());

  std::string notifPath(path("alarm-notif"));
  std::string alarmPath = notifPath + "/alarm";

  params->addParam(std::to_string(alarm->fault_id), alarmPath + "/fault-id");
  if (alarm->fault_source)
    params->addParam(std::string(alarm->fault_source), alarmPath + "/fault-source");

  const char* sev = "MAJOR";
  switch (alarm->fault_severity) {
    case ORAN_FAULT_SEVERITY_CRITICAL: sev = "CRITICAL"; break;
    case ORAN_FAULT_SEVERITY_MAJOR: sev = "MAJOR"; break;
    case ORAN_FAULT_SEVERITY_MINOR: sev = "MINOR"; break;
    case ORAN_FAULT_SEVERITY_WARNING: sev = "WARNING"; break;
  }
  params->addParam(std::string(sev), alarmPath + "/fault-severity");
  params->addParam(alarm->is_cleared ? "true" : "false", alarmPath + "/is-cleared");
  if (alarm->fault_text)
    params->addParam(std::string(alarm->fault_text), alarmPath + "/fault-text");

  params->addParam(iso8601_now(), alarmPath + "/event-time");

  sendNotification(notifPath, params);
}

