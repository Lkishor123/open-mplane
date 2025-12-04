/*
 * OranSwmHandler: publishes o-ran-software-management notifications and maintains inventory
 */

#include <map>
#include <string>
#include <thread>

#include "OranSwmHandler.h"
#include "CallbackORanSoftwareMgr.h"

// Required includes for RPC handlers
#include "Path.h"
#include "ISftpMgr.h"
#include "ISoftwareSlotMgr.h"
#include "XpathUtils.h"
#include "YangParamUtils.h"
#include "SysrepoUtils.h"
#include "YangUserAuth.h"
#include "stringfunc.hpp"

using namespace Mplane;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

#ifdef HAL_TEST
const std::string DIR_PREFIX("/tmp");
#else
const std::string DIR_PREFIX("");
#endif

//=============================================================================================================
// STATIC
//=============================================================================================================

#ifdef HAL_TEST
static OranSwmHandler* g_oranSwmHandler = nullptr;
#endif

//=============================================================================================================
// PUBLIC
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
OranSwmHandler::OranSwmHandler(std::shared_ptr<IYangModuleMgr> moduleMgr)
  : YangHandlerSysrepo("o-ran-software-management", moduleMgr, "OranSwmHandler"),
    mCallback(std::make_shared<CallbackORanSoftwareMgr>(path("software-inventory"))),
    mSoftwareDir(DIR_PREFIX + "/O-RAN/software")
{
  // Ensure download directory exists
  Path::mkpath(mSoftwareDir);
}

//-------------------------------------------------------------------------------------------------------------
OranSwmHandler::~OranSwmHandler()
{
  if (g_oranSwmHandler == this)
    g_oranSwmHandler = nullptr;
}

//-------------------------------------------------------------------------------------------------------------
bool OranSwmHandler::initialise() {
  g_oranSwmHandler = this;

#ifdef HAL_TEST
  // Register HAL callback for shim commands (legacy)
  mock_hal_register_sw_cb(&OranSwmHandler::swCallbackWrapper);
#endif

  // Register operational data callback for software-inventory queries
  getItemsSubscribe(path("software-inventory"), mCallback);
  changeSubscribe();

  // Register RPC callbacks
  REGISTER_RPC("software-download", OranSwmHandler::rpcDownload);
  REGISTER_RPC("software-install", OranSwmHandler::rpcInstall);
  REGISTER_RPC("software-activate", OranSwmHandler::rpcActivate);

  return true;
}

//=============================================================================================================
// RPC HANDLERS
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
bool OranSwmHandler::rpcDownload(
    std::shared_ptr<sysrepo::Session> session,
    const std::string& rpcXpath,
    std::shared_ptr<YangParams> callList,
    std::shared_ptr<YangParams> retList)
{
    const unsigned downloadTimeout = 60;
    const std::string rpc = "software-download";

    std::map<std::string, std::shared_ptr<YangParam>> args(
        SysrepoUtils::paramsToMap(callList));

    std::string error;
    std::string remotePath = YangParamUtils::toString(args["remote-file-path"], error);
    std::string localPath = mSoftwareDir + "/" + Path::basename(remotePath);

    // Get authentication parameters
    std::string authData;
    ISftpMgr::AuthenticationType authType = ISftpMgr::AUTH_NONE;
    std::vector<ISshSession::PublicKey> serverKeys;

    if (!YangUserAuth::authParams(callList, authData, authType, serverKeys)) {
        statusFail(rpc, retList, "Failed to get authentication parameters");
        return true;
    }

    if (authType == ISftpMgr::AUTH_NONE) {
        statusFail(rpc, retList, "No supported authorisation method provided");
        return true;
    }

    // Create SFTP session
    std::shared_ptr<ISftpSession> sftp(
        ISftpMgr::factory(remotePath, authData, authType, serverKeys, error));

    if (!sftp) {
        statusFail(rpc, retList, error);
        return true;
    }

    // Setup completion callback
    auto complete = [this](
        const std::string& remoteFile,
        const std::string& localFile,
        ISftpSession::SftpTransaction transaction,
        const std::string& error,
        bool timedOut) {

        // Transfer complete so send notification
        const std::string notifPath = path("download-event");
        auto params = std::make_shared<YangParams>();

        params->addParam(localFile, notifPath + "/file-name");

        if (error.empty()) {
            params->addParam("COMPLETED", notifPath + "/status");
        } else if (timedOut) {
            params->addParam("TIMEOUT", notifPath + "/status");
            params->addParam(error, notifPath + "/error-message");
        } else {
            params->addParam("PROTOCOL_ERROR", notifPath + "/status");
            params->addParam(error, notifPath + "/error-message");
        }

        sendNotification(notifPath, params);
    };

    // Start download
    if (!sftp->fileDownload(remotePath, localPath, complete, downloadTimeout)) {
        statusFail(rpc, retList, sftp->error());
        return true;
    }

    // Return success
    statusOk(rpc, retList);
    retList->addParam(std::to_string(downloadTimeout),
                      path(rpc + "/notification-timeout"));

    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool OranSwmHandler::rpcInstall(
    std::shared_ptr<sysrepo::Session> session,
    const std::string& rpcXpath,
    std::shared_ptr<YangParams> callList,
    std::shared_ptr<YangParams> retList)
{
    const std::string rpc = "software-install";
    std::map<std::string, std::shared_ptr<YangParam>> args(
        SysrepoUtils::paramsToMap(callList));

    std::string error;
    std::string slotName = YangParamUtils::toString(args["slot-name"], error);

    // Collect file names
    std::vector<std::string> files;
    for (unsigned i = 0; i < callList->getNumParams(); ++i) {
        std::shared_ptr<YangParam> p(callList->getParam(i));
        std::string leaf(XpathUtils::leafName(p->name()));
        if (!startsWith(leaf, "file-names"))
            continue;

        std::string file = YangParamUtils::toString(p, error);
        // Ensure file uses download directory
        file = mSoftwareDir + "/" + Path::basename(file);
        files.push_back(file);
    }

    // Setup completion callback
    auto complete = [this](
        const std::string& slotName,
        ISoftwareSlotMgr::InstallStatus status,
        const std::string& error) {

        const std::string notifPath = path("install-event");
        auto params = std::make_shared<YangParams>();

        params->addParam(slotName, notifPath + "/slot-name");
        params->addParam(ISoftwareSlotMgr::installStatusString(status),
                        notifPath + "/status");

        if (!error.empty()) {
            params->addParam(error, notifPath + "/error-message");
        }

        sendNotification(notifPath, params);
    };

    // Start install
    std::shared_ptr<ISoftwareSlotMgr> mgr(ISoftwareSlotMgr::singleton());
    if (!mgr->install(slotName, files, complete, error)) {
        statusFail(rpc, retList, error);
        return true;
    }

    // Return success
    statusOk(rpc, retList);
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool OranSwmHandler::rpcActivate(
    std::shared_ptr<sysrepo::Session> session,
    const std::string& rpcXpath,
    std::shared_ptr<YangParams> callList,
    std::shared_ptr<YangParams> retList)
{
    const std::string rpc = "software-activate";
    std::map<std::string, std::shared_ptr<YangParam>> args(
        SysrepoUtils::paramsToMap(callList));

    std::string error;
    std::string slotName = YangParamUtils::toString(args["slot-name"], error);

    // Setup completion callback
    auto complete = [this](
        const std::string& slotName,
        ISoftwareSlotMgr::ActivateStatus status,
        unsigned returnCode,
        const std::string& error) {

        const std::string notifPath = path("activation-event");
        auto params = std::make_shared<YangParams>();

        params->addParam(slotName, notifPath + "/slot-name");
        params->addParam(std::to_string(returnCode), notifPath + "/return-code");
        params->addParam(ISoftwareSlotMgr::activateStatusString(status),
                        notifPath + "/status");

        if (!error.empty()) {
            params->addParam(error, notifPath + "/error-message");
        }

        sendNotification(notifPath, params);
    };

    // Start activation
    unsigned requiredTimeoutSecs;
    std::shared_ptr<ISoftwareSlotMgr> mgr(ISoftwareSlotMgr::singleton());
    if (!mgr->activate(slotName, requiredTimeoutSecs, complete, error)) {
        statusFail(rpc, retList, error);
        return true;
    }

    // Return success
    statusOk(rpc, retList);
    retList->addParam(requiredTimeoutSecs, path(rpc + "/notification-timeout"));
    return true;
}

//=============================================================================================================
// HELPER METHODS
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
void OranSwmHandler::statusOk(const std::string& rpc,
                               std::shared_ptr<YangParams> retList) {
    retList->addParam("STARTED", path(rpc + "/status"));
}

//-------------------------------------------------------------------------------------------------------------
void OranSwmHandler::statusFail(const std::string& rpc,
                                 std::shared_ptr<YangParams> retList,
                                 const std::string& reason,
                                 const std::string& status) {
    retList->addParam(status, path(rpc + "/status"));
    retList->addParam(reason, path(rpc + "/error-message"));
}

//=============================================================================================================
// LEGACY HAL CALLBACK (for shim commands)
//=============================================================================================================

#ifdef HAL_TEST
//-------------------------------------------------------------------------------------------------------------
void OranSwmHandler::swCallbackWrapper(const mock_sw_event_t* ev)
{
  if (g_oranSwmHandler) g_oranSwmHandler->handleSwEvent(ev);
}

//-------------------------------------------------------------------------------------------------------------
const char* OranSwmHandler::normalize_status(const char* res)
{
  if (!res) return "FAILED";
  if (std::string(res) == "OK") return "COMPLETED";
  return res;
}

//-------------------------------------------------------------------------------------------------------------
void OranSwmHandler::handleSwEvent(const mock_sw_event_t* ev)
{
  if (!ev || !ev->phase || !ev->result) return;
  std::string phase(ev->phase);
  std::string resultStr(normalize_status(ev->result));

  // Send notifications (legacy shim command support)
  if (phase == "download") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("download-event");
    if (ev->file) params->addParam(std::string(ev->file), notif + "/file-name");
    params->addParam(resultStr, notif + "/status");
    sendNotification(notif, params);
  } else if (phase == "install") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("install-event");
    params->addParam(resultStr, notif + "/status");
    if (ev->slot) params->addParam(std::string(ev->slot), notif + "/slot-name");
    sendNotification(notif, params);
  } else if (phase == "activate") {
    auto params = std::make_shared<YangParams>();
    std::string notif = path("activation-event");
    params->addParam(resultStr, notif + "/status");
    if (ev->slot) params->addParam(std::string(ev->slot), notif + "/slot-name");
    sendNotification(notif, params);
  }
}
#endif
