/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef SIMSOFTWARESLOT_H_
#define SIMSOFTWARESLOT_H_

#include <string>
#include <memory>
#include <iostream>

#include "ISoftwareSlot.h"
#include "ISoftwarePackage.h"

namespace Mplane {

/*!
 * \class SimSoftwareSlot
 * \brief Simulator implementation of ISoftwareSlot
 * \details Represents a software slot that can hold a package
 */
class SimSoftwareSlot : public ISoftwareSlot {
public:
    /**
     * Constructor
     * @param name Slot name (e.g., "slot-0", "slot-1")
     */
    explicit SimSoftwareSlot(const std::string& name);
    virtual ~SimSoftwareSlot() = default;

    // Basic info
    std::string name() const override { return mName; }

    // State queries
    bool isAllocated() const override { return mPackage != nullptr; }
    bool isValid() const override;
    bool isActive() const override;
    bool isRunning() const override;
    bool isReadOnly() const override;

    // State modifications
    bool activate() override;
    bool setReadOnly(bool readOnly) override;

    // Package management
    bool allocatePackage(std::shared_ptr<ISoftwarePackage> pkg) override;
    std::shared_ptr<ISoftwarePackage> package() const override { return mPackage; }
    bool clearPackage() override;

    void show(std::ostream& os = std::cout) override;

private:
    std::string mName;
    std::shared_ptr<ISoftwarePackage> mPackage;
};

} // namespace Mplane

#endif /* SIMSOFTWARESLOT_H_ */
