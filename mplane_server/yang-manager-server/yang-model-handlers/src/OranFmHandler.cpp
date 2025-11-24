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
  if (g_oranFmHandler) {
    g_oranFmHandler->handleAlarm(alarm);
  }
}

void OranFmHandler::handleAlarm(const halmplane_oran_alarm_t* alarm)
{
  // Build notification params
  std::shared_ptr<YangParams> params(std::make_shared<YangParams>());

  std::string notifPath(path("alarm-notif"));

  // Note: alarm fields are directly under alarm-notif
  params->addParam(std::to_string(alarm->fault_id), notifPath + "/fault-id");

  // fault-source is mandatory
  std::string faultSource = alarm->fault_source ? std::string(alarm->fault_source) : "UNKNOWN";
  params->addParam(faultSource, notifPath + "/fault-source");

  const char* sev = "MAJOR";
  switch (alarm->fault_severity) {
    case ORAN_FAULT_SEVERITY_CRITICAL: sev = "CRITICAL"; break;
    case ORAN_FAULT_SEVERITY_MAJOR: sev = "MAJOR"; break;
    case ORAN_FAULT_SEVERITY_MINOR: sev = "MINOR"; break;
    case ORAN_FAULT_SEVERITY_WARNING: sev = "WARNING"; break;
  }
  params->addParam(std::string(sev), notifPath + "/fault-severity");
  params->addParam(alarm->is_cleared ? "true" : "false", notifPath + "/is-cleared");

  if (alarm->fault_text)
    params->addParam(std::string(alarm->fault_text), notifPath + "/fault-text");

  params->addParam(iso8601_now(), notifPath + "/event-time");

  // affected-objects is mandatory (min-elements 1) - use array index [1] not key predicate
  params->addParam(faultSource, notifPath + "/affected-objects[1]/name");
  sendNotification(notifPath, params);
}

