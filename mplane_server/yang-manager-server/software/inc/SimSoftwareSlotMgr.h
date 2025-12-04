/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef SIMSOFTWARESLOTMGR_H_
#define SIMSOFTWARESLOTMGR_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "ISoftwareSlotMgr.h"
#include "ISoftwareSlot.h"

namespace Mplane {

/*!
 * \class SimSoftwareSlotMgr
 * \brief Simulator implementation of ISoftwareSlotMgr
 * \details Manages software slots and orchestrates install/activate operations
 */
class SimSoftwareSlotMgr : public ISoftwareSlotMgr {
public:
    SimSoftwareSlotMgr();
    virtual ~SimSoftwareSlotMgr() = default;

    // Singleton access
    static std::shared_ptr<SimSoftwareSlotMgr> getInstance();

    // Slot access
    std::vector<std::shared_ptr<ISoftwareSlot>> slots() const override;

    // Operations
    bool install(
        const std::string& slotName,
        const std::vector<std::string>& files,
        InstallCallback func,
        std::string& error) override;

    bool activate(
        const std::string& slotName,
        unsigned& requiredTimeoutSecs,
        ActivateCallback func,
        std::string& error) override;

    // Utility
    void show(std::ostream& os = std::cout) override;
    bool clean(std::string& error) override;
    bool setReadonly(const std::string& slotName, bool readOnly, std::string& error) override;

private:
    // Find slot by name
    std::shared_ptr<ISoftwareSlot> findSlot(const std::string& name);

    // Background worker methods
    void installWorker(
        const std::string& slotName,
        const std::vector<std::string>& files,
        InstallCallback func);

    void activateWorker(
        const std::string& slotName,
        ActivateCallback func);

    std::vector<std::shared_ptr<ISoftwareSlot>> mSlots;

    static std::shared_ptr<SimSoftwareSlotMgr> mInstance;
};

} // namespace Mplane

#endif /* SIMSOFTWARESLOTMGR_H_ */
