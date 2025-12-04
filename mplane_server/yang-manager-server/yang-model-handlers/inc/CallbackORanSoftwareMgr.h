/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * \file      CallbackORanSoftwareMgr.h
 * \brief     Callback to populate software-inventory operational data
 *
 *
 * \details   Reads from ISoftwareSlotMgr and populates YANG tree when
 *            client queries software-inventory
 *
 */

#ifndef CALLBACKORANSOFTWAREMGR_H_
#define CALLBACKORANSOFTWAREMGR_H_

#include <sysrepo-cpp/Connection.hpp>
#include <sysrepo-cpp/Session.hpp>
#include <sysrepo-cpp/Sysrepo.hpp>

#include <map>
#include <string>
#include <vector>

namespace Mplane {

/*!
 * \class  CallbackORanSoftwareMgr
 * \brief  Callback for software-inventory operational data
 * \details Inherits from sysrepo::Callback and implements oper_get_items
 *          to populate software inventory when client queries via <get>
 */
class CallbackORanSoftwareMgr : public sysrepo::Callback {
public:
    /**
     * Constructor
     * @param parentPath XPath to the software-inventory parent node
     */
    explicit CallbackORanSoftwareMgr(const std::string& parentPath);
    virtual ~CallbackORanSoftwareMgr();

    /**
     * Get the path this callback handles
     */
    virtual std::string path() const;

    /**
     * Called by sysrepo when client queries software-inventory
     * Populates the libyang tree with current slot state
     */
    virtual int oper_get_items(
        sysrepo::S_Session session,
        const char* module_name,
        const char* path,
        const char* request_xpath,
        uint32_t request_id,
        libyang::S_Data_Node& parent,
        void* private_data) override;

private:
    /**
     * Process and populate software-slot elements
     */
    void processElements(
        libyang::S_Context ctx,
        libyang::S_Data_Node parent,
        libyang::S_Module mod);

    std::string mParentPath; //!> XPath to the parent node (software-inventory)
};

} // namespace Mplane

#endif /* CALLBACKORANSOFTWAREMGR_H_ */