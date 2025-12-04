/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef SIMSOFTWAREPACKAGE_H_
#define SIMSOFTWAREPACKAGE_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "ISoftwarePackage.h"

namespace Mplane {

/*!
 * \class SimSoftwarePackage
 * \brief Simulator implementation of ISoftwarePackage
 * \details Represents a software image file with mock validation
 */
class SimSoftwarePackage : public ISoftwarePackage {
public:
    /**
     * Constructor from file list
     * @param files List of file paths that make up this package
     */
    explicit SimSoftwarePackage(const std::vector<std::string>& files);
    virtual ~SimSoftwarePackage() = default;

    // State queries
    bool isValid() const override { return mValid; }
    bool isActive() const override { return mActive; }
    bool isRunning() const override { return mRunning; }
    bool isReadOnly() const override { return mReadOnly; }

    // State modifications
    bool activate() override;
    bool setReadOnly(bool readOnly) override;
    bool installImage(unsigned timeoutSecs, std::string& error) override;
    bool createMd5Files(std::string& error) override;

    // Manifest API - Mock values for simulator
    std::string manifestPath() const override { return mManifestPath; }
    std::string vendorCode() const override { return "ORAN-SIM"; }
    std::string productCode() const override { return "SIMULATOR-RU"; }
    std::string buildName() const override { return mBuildName; }
    std::string buildVersion() const override { return mBuildVersion; }
    std::string buildId() const override { return mBuildId; }

    // OS file info (first file in list)
    std::string osFileName() const override;
    std::string osFileVersion() const override { return "1.0.0-sim"; }
    std::string osPath() const override;
    std::string osChecksum() const override;
    bool osIntegrity() const override { return mValid; }

    // App file info (second file in list, if exists)
    std::string appFileName() const override;
    std::string appFileVersion() const override { return "1.0.0-sim"; }
    std::string appPath() const override;
    std::string appChecksum() const override;
    bool appIntegrity() const override { return mValid; }

    unsigned requiredInstallTimeSecs() const override { return 30; }
    void show(std::ostream& os = std::cout) override;

    // Internal setters (for slot manager use)
    void setValid(bool valid) { mValid = valid; }
    void setActive(bool active) { mActive = active; }
    void setRunning(bool running) { mRunning = running; }

    const std::vector<std::string>& files() const { return mFiles; }

private:
    std::vector<std::string> mFiles;
    std::string mManifestPath;
    std::string mBuildName;
    std::string mBuildVersion;
    std::string mBuildId;
    bool mValid;
    bool mActive;
    bool mRunning;
    bool mReadOnly;

    // Helper to generate mock manifest path
    void generateManifestPath();
    // Helper to extract build info from filename
    void extractBuildInfo();
};

} // namespace Mplane

#endif /* SIMSOFTWAREPACKAGE_H_ */
