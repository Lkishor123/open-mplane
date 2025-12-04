/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "SimSoftwareSlotMgr.h"
#include "SimSoftwareSlot.h"
#include "SimSoftwarePackage.h"
#include "Path.h"

#include <thread>
#include <chrono>
#include <algorithm>

using namespace Mplane;

//=============================================================================================================
// STATIC
//=============================================================================================================

std::shared_ptr<SimSoftwareSlotMgr> SimSoftwareSlotMgr::mInstance;

//-------------------------------------------------------------------------------------------------------------
std::shared_ptr<SimSoftwareSlotMgr> SimSoftwareSlotMgr::getInstance()
{
    if (!mInstance) {
        mInstance = std::make_shared<SimSoftwareSlotMgr>();
    }
    return mInstance;
}

//-------------------------------------------------------------------------------------------------------------
std::shared_ptr<ISoftwareSlotMgr> ISoftwareSlotMgr::singleton()
{
    return SimSoftwareSlotMgr::getInstance();
}

//-------------------------------------------------------------------------------------------------------------
std::string ISoftwareSlotMgr::installStatusString(InstallStatus status)
{
    switch (status) {
        case INSTALL_COMPLETED:
            return "COMPLETED";
        case INSTALL_FILE_ERROR:
            return "FILE_ERROR";
        case INSTALL_INTEGRITY_ERROR:
            return "INTEGRITY_ERROR";
        case INSTALL_APPLICATION_ERROR:
            return "APPLICATION_ERROR";
        default:
            return "UNKNOWN";
    }
}

//-------------------------------------------------------------------------------------------------------------
std::string ISoftwareSlotMgr::activateStatusString(ActivateStatus status)
{
    switch (status) {
        case ACTIVATE_COMPLETED:
            return "COMPLETED";
        case ACTIVATE_APPLICATION_ERROR:
            return "APPLICATION_ERROR";
        default:
            return "UNKNOWN";
    }
}

//=============================================================================================================
// PUBLIC
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
SimSoftwareSlotMgr::SimSoftwareSlotMgr()
{
    // Create 2 slots as required by O-RAN spec (min-elements 2)
    mSlots.push_back(std::make_shared<SimSoftwareSlot>("slot-0"));
    mSlots.push_back(std::make_shared<SimSoftwareSlot>("slot-1"));

    // Optionally: Make slot-0 the "running" slot with dummy current software
    // This would require creating a default package
}

//-------------------------------------------------------------------------------------------------------------
std::vector<std::shared_ptr<ISoftwareSlot>> SimSoftwareSlotMgr::slots() const
{
    // No lock needed in simulator - keep it simple
    return mSlots;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlotMgr::install(
    const std::string& slotName,
    const std::vector<std::string>& files,
    InstallCallback func,
    std::string& error)
{
    // No lock needed in simulator - keep it simple

    // Validate slot exists
    auto slot = findSlot(slotName);
    if (!slot) {
        error = "Slot not found: " + slotName;
        return false;
    }

    // Validate slot is not active or running (O-RAN constraint)
    if (slot->isActive() || slot->isRunning()) {
        error = "Cannot install to active or running slot: " + slotName;
        return false;
    }

    // Validate files list not empty
    if (files.empty()) {
        error = "No files specified for installation";
        return false;
    }

    // Spawn background thread to do the work
    std::thread worker([this, slotName, files, func]() {
        installWorker(slotName, files, func);
    });
    worker.detach();

    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlotMgr::activate(
    const std::string& slotName,
    unsigned& requiredTimeoutSecs,
    ActivateCallback func,
    std::string& error)
{
    // No lock needed in simulator - keep it simple

    // Validate slot exists
    auto slot = findSlot(slotName);
    if (!slot) {
        error = "Slot not found: " + slotName;
        return false;
    }

    // Validate slot is VALID (O-RAN constraint)
    if (!slot->isValid()) {
        error = "Cannot activate slot that is not VALID: " + slotName;
        return false;
    }

    // Set timeout (in simulator, activation is quick)
    requiredTimeoutSecs = 30;

    // Spawn background thread to do the work
    std::thread worker([this, slotName, func]() {
        activateWorker(slotName, func);
    });
    worker.detach();

    return true;
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwareSlotMgr::show(std::ostream& os)
{
    // No lock needed in simulator - keep it simple

    os << "=== Software Slot Manager ===" << std::endl;
    os << "Total slots: " << mSlots.size() << std::endl;
    os << std::endl;

    for (auto& slot : mSlots) {
        slot->show(os);
        os << std::endl;
    }
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlotMgr::clean(std::string& /*error*/)
{
    // In simulator, nothing to clean
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlotMgr::setReadonly(const std::string& slotName, bool readOnly, std::string& error)
{
    // No lock needed in simulator - keep it simple

    auto slot = findSlot(slotName);
    if (!slot) {
        error = "Slot not found: " + slotName;
        return false;
    }

    return slot->setReadOnly(readOnly);
}

//=============================================================================================================
// PRIVATE
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
std::shared_ptr<ISoftwareSlot> SimSoftwareSlotMgr::findSlot(const std::string& name)
{
    // Called with lock already held
    auto it = std::find_if(mSlots.begin(), mSlots.end(),
        [&name](const std::shared_ptr<ISoftwareSlot>& slot) {
            return slot->name() == name;
        });

    if (it == mSlots.end()) {
        return nullptr;
    }

    return *it;
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwareSlotMgr::installWorker(
    const std::string& slotName,
    const std::vector<std::string>& files,
    InstallCallback func)
{
    // This runs in background thread

    InstallStatus status = INSTALL_COMPLETED;
    std::string error;

    // Simulate installation delay
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Create package from files
    auto package = std::make_shared<SimSoftwarePackage>(files);

    // Validate files exist
    for (const auto& file : files) {
        if (!Path::exists(file)) {
            status = INSTALL_FILE_ERROR;
            error = "File not found: " + file;
            func(slotName, status, error);
            return;
        }
    }

    // Install the package (validates and marks valid)
    std::string installError;
    if (!package->installImage(30, installError)) {
        status = INSTALL_FILE_ERROR;
        error = installError;
        func(slotName, status, error);
        return;
    }

    // Allocate package to slot
    // No lock needed in simulator - keep it simple
    auto slot = findSlot(slotName);
    if (!slot) {
        status = INSTALL_APPLICATION_ERROR;
        error = "Slot disappeared during installation: " + slotName;
        func(slotName, status, error);
        return;
    }

    slot->allocatePackage(package);

    // Success
    func(slotName, INSTALL_COMPLETED, "");
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwareSlotMgr::activateWorker(
    const std::string& slotName,
    ActivateCallback func)
{
    // This runs in background thread

    ActivateStatus status = ACTIVATE_COMPLETED;
    std::string error;
    unsigned returnCode = 0;

    // Simulate activation delay
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // No lock needed in simulator - keep it simple

    // Find target slot
    auto targetSlot = findSlot(slotName);
    if (!targetSlot) {
        status = ACTIVATE_APPLICATION_ERROR;
        error = "Slot not found: " + slotName;
        func(slotName, status, returnCode, error);
        return;
    }

    // Deactivate all other slots (only one can be active)
    for (auto& slot : mSlots) {
        if (slot->name() != slotName && slot->isAllocated()) {
            auto pkg = std::dynamic_pointer_cast<SimSoftwarePackage>(slot->package());
            if (pkg) {
                pkg->setActive(false);
            }
        }
    }

    // Activate target slot
    if (!targetSlot->activate()) {
        status = ACTIVATE_APPLICATION_ERROR;
        error = "Failed to activate slot (not valid?)";
        func(slotName, status, returnCode, error);
        return;
    }

    // Success
    func(slotName, ACTIVATE_COMPLETED, returnCode, "");
}