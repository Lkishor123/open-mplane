/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "SimSoftwareSlot.h"
#include <sstream>

using namespace Mplane;

//=============================================================================================================
// PUBLIC
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
SimSoftwareSlot::SimSoftwareSlot(const std::string& name)
    : mName(name),
      mPackage(nullptr)
{
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::isValid() const
{
    if (!mPackage) {
        return false;
    }
    return mPackage->isValid();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::isActive() const
{
    if (!mPackage) {
        return false;
    }
    return mPackage->isActive();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::isRunning() const
{
    if (!mPackage) {
        return false;
    }
    return mPackage->isRunning();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::isReadOnly() const
{
    if (!mPackage) {
        return false;
    }
    return mPackage->isReadOnly();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::activate()
{
    if (!mPackage) {
        return false;
    }

    if (!mPackage->isValid()) {
        return false;
    }

    return mPackage->activate();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::setReadOnly(bool readOnly)
{
    if (!mPackage) {
        return false;
    }

    return mPackage->setReadOnly(readOnly);
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::allocatePackage(std::shared_ptr<ISoftwarePackage> pkg)
{
    mPackage = pkg;
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwareSlot::clearPackage()
{
    mPackage.reset();
    return true;
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwareSlot::show(std::ostream& os)
{
    os << "Slot: " << mName << std::endl;

    if (!mPackage) {
        os << "  Status: EMPTY" << std::endl;
        return;
    }

    os << "  Status: " << (isValid() ? "VALID" : "INVALID") << std::endl;
    os << "  Active: " << (isActive() ? "true" : "false") << std::endl;
    os << "  Running: " << (isRunning() ? "true" : "false") << std::endl;
    os << "  Package:" << std::endl;

    // Indent package output
    std::ostringstream pkgStream;
    mPackage->show(pkgStream);
    std::string pkgStr = pkgStream.str();

    std::istringstream iss(pkgStr);
    std::string line;
    while (std::getline(iss, line)) {
        os << "    " << line << std::endl;
    }
}