/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * \file      CallbackORanSoftwareMgr.cpp
 * \brief     Implementation of software-inventory callback
 *
 */

#include "CallbackORanSoftwareMgr.h"
#include "ISoftwareSlotMgr.h"
#include "ISoftwareSlot.h"
#include "ISoftwarePackage.h"
#include "Path.h"

#include <iostream>

using namespace Mplane;

//=============================================================================================================
// PUBLIC
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
CallbackORanSoftwareMgr::CallbackORanSoftwareMgr(const std::string& parentPath)
    : mParentPath(parentPath)
{
}

//-------------------------------------------------------------------------------------------------------------
CallbackORanSoftwareMgr::~CallbackORanSoftwareMgr()
{
}

//-------------------------------------------------------------------------------------------------------------
std::string CallbackORanSoftwareMgr::path() const
{
    return mParentPath;
}

//-------------------------------------------------------------------------------------------------------------
int CallbackORanSoftwareMgr::oper_get_items(
    sysrepo::S_Session session,
    const char* module_name,
    const char* path,
    const char* request_xpath,
    uint32_t request_id,
    libyang::S_Data_Node& parent,
    void* private_data)
{
    try {
        // Get libyang context and module
        libyang::S_Context ctx = session->get_context();
        libyang::S_Module mod = ctx->get_module(module_name);

        if (!mod) {
            std::cerr << "CallbackORanSoftwareMgr: Failed to get module: " << module_name << std::endl;
            return SR_ERR_OPERATION_FAILED;
        }

        // IMPORTANT: Reset parent to create the root container first
        // This is required for sysrepo to properly return the data
        parent.reset(new libyang::Data_Node(
            ctx, mParentPath.c_str(), nullptr, LYD_ANYDATA_CONSTSTRING, 0));

        // Populate the tree with software slot data
        processElements(ctx, parent, mod);

        return SR_ERR_OK;

    } catch (std::exception& e) {
        std::cerr << "[ERROR] CallbackORanSoftwareMgr::oper_get_items exception: "
                  << e.what() << std::endl;
        return SR_ERR_INTERNAL;
    }
}

//=============================================================================================================
// PRIVATE
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
void CallbackORanSoftwareMgr::processElements(
    libyang::S_Context ctx,
    libyang::S_Data_Node parent,
    libyang::S_Module mod)
{
    std::cout << "[DEBUG-CallbackORanSoftwareMgr] processElements() called" << std::endl;

    // Get slot manager singleton
    std::cout << "[DEBUG-CallbackORanSoftwareMgr] Getting ISoftwareSlotMgr singleton..." << std::endl;
    std::shared_ptr<ISoftwareSlotMgr> mgr(ISoftwareSlotMgr::singleton());
    if (!mgr) {
        std::cerr << "CallbackORanSoftwareMgr: Failed to get ISoftwareSlotMgr singleton" << std::endl;
        return;
    }
    std::cout << "[DEBUG-CallbackORanSoftwareMgr] Got singleton successfully" << std::endl;

    // Get all slots
    std::cout << "[DEBUG-CallbackORanSoftwareMgr] Getting slots..." << std::endl;
    std::vector<std::shared_ptr<ISoftwareSlot>> slots(mgr->slots());
    std::cout << "[DEBUG-CallbackORanSoftwareMgr] Got " << slots.size() << " slots" << std::endl;

    // Populate software-inventory with slot data
    for (auto slot : slots) {
        std::cout << "[DEBUG-CallbackORanSoftwareMgr] Processing slot: " << slot->name() << std::endl;

        try {
            // Create list entry as child of parent container
            // Use the parent-based constructor, not path-based
            libyang::S_Data_Node lyslot(new libyang::Data_Node(parent, mod, "software-slot"));

            if (!lyslot) {
                std::cerr << "[ERROR] Failed to create slot node for: " << slot->name() << std::endl;
                continue;
            }

            std::cout << "[DEBUG-CallbackORanSoftwareMgr] Created software-slot node for: " << slot->name() << std::endl;

            // Add name (key) - must be first!
            libyang::S_Data_Node nameNode(new libyang::Data_Node(
                lyslot, mod, "name", slot->name().c_str()));

            // Determine status (EMPTY, INVALID, VALID)
            std::string statusStr("EMPTY");
            if (slot->isAllocated()) {
                if (slot->isValid())
                    statusStr = "VALID";
                else
                    statusStr = "INVALID";
            }

            // Add status leaf as child of slot node
            libyang::S_Data_Node status(new libyang::Data_Node(
                lyslot, mod, "status", statusStr.c_str()));

            // Add active flag (only if status=VALID per YANG constraint)
            if (slot->isValid()) {
                libyang::S_Data_Node active(new libyang::Data_Node(
                    lyslot, mod, "active", (slot->isActive() ? "true" : "false")));

                // Add running flag (only if status=VALID per YANG constraint)
                libyang::S_Data_Node running(new libyang::Data_Node(
                    lyslot, mod, "running", (slot->isRunning() ? "true" : "false")));
            }

            // Add access (read-only status)
            const char* accessStr = slot->isReadOnly() ? "READ_ONLY" : "READ_WRITE";
            libyang::S_Data_Node access(new libyang::Data_Node(
                lyslot, mod, "access", accessStr));

            // If slot has a package, add detailed info (simplified for now)
            if (slot->isAllocated()) {
                auto pkg = slot->package();
                if (pkg) {
                    // Add basic package info as children of slot node
                    std::string vendorCode = pkg->vendorCode();
                    if (!vendorCode.empty()) {
                        libyang::S_Data_Node vendor(new libyang::Data_Node(
                            lyslot, mod, "vendor-code", vendorCode.c_str()));
                    }

                    std::string productCode = pkg->productCode();
                    if (!productCode.empty()) {
                        libyang::S_Data_Node product(new libyang::Data_Node(
                            lyslot, mod, "product-code", productCode.c_str()));
                    }

                    std::string buildId = pkg->buildId();
                    if (!buildId.empty()) {
                        libyang::S_Data_Node build(new libyang::Data_Node(
                            lyslot, mod, "build-id", buildId.c_str()));
                    }

                    std::string buildName = pkg->buildName();
                    if (!buildName.empty()) {
                        libyang::S_Data_Node buildNameNode(new libyang::Data_Node(
                            lyslot, mod, "build-name", buildName.c_str()));
                    }

                    std::string buildVersion = pkg->buildVersion();
                    if (!buildVersion.empty()) {
                        libyang::S_Data_Node buildVer(new libyang::Data_Node(
                            lyslot, mod, "build-version", buildVersion.c_str()));
                    }
                }
            }

            std::cout << "[DEBUG-CallbackORanSoftwareMgr] Successfully populated slot: " << slot->name() << std::endl;

        } catch (std::exception& e) {
            std::cerr << "[ERROR-CallbackORanSoftwareMgr] Exception processing slot "
                      << slot->name() << ": " << e.what() << std::endl;
            // Continue with next slot
        }
    }

    std::cout << "[DEBUG-CallbackORanSoftwareMgr] Finished processing all slots" << std::endl;
}