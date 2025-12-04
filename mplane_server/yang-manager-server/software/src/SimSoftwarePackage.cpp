/*!
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "SimSoftwarePackage.h"
#include "Path.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace Mplane;

//=============================================================================================================
// PUBLIC
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
SimSoftwarePackage::SimSoftwarePackage(const std::vector<std::string>& files)
    : mFiles(files),
      mManifestPath(""),
      mBuildName("simulator-build"),
      mBuildVersion("1.0.0"),
      mBuildId("sim-001"),
      mValid(false),
      mActive(false),
      mRunning(false),
      mReadOnly(false)
{
    generateManifestPath();
    extractBuildInfo();
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwarePackage::activate()
{
    if (!mValid) {
        return false;
    }
    mActive = true;
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwarePackage::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwarePackage::installImage(unsigned timeoutSecs, std::string& error)
{
    // Simulate installation delay
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Validate all files exist
    for (const auto& file : mFiles) {
        if (!Path::exists(file)) {
            error = "File not found: " + file;
            mValid = false;
            return false;
        }
    }

    // In real implementation, would:
    // - Validate MD5 checksums
    // - Parse manifest.json
    // - Validate file integrity
    // - Flash to storage

    // For simulator, just mark as valid
    mValid = true;
    error.clear();
    return true;
}

//-------------------------------------------------------------------------------------------------------------
bool SimSoftwarePackage::createMd5Files(std::string& error)
{
    // Mock implementation - in real code would generate MD5 checksums
    return true;
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::osFileName() const
{
    if (mFiles.empty()) {
        return "";
    }
    return Path::basename(mFiles[0]);
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::osPath() const
{
    if (mFiles.empty()) {
        return "";
    }
    return mFiles[0];
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::osChecksum() const
{
    // Mock checksum - in real implementation would compute MD5
    if (mFiles.empty()) {
        return "";
    }
    return "abcdef1234567890abcdef1234567890";
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::appFileName() const
{
    if (mFiles.size() < 2) {
        return "";
    }
    return Path::basename(mFiles[1]);
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::appPath() const
{
    if (mFiles.size() < 2) {
        return "";
    }
    return mFiles[1];
}

//-------------------------------------------------------------------------------------------------------------
std::string SimSoftwarePackage::appChecksum() const
{
    // Mock checksum - in real implementation would compute MD5
    if (mFiles.size() < 2) {
        return "";
    }
    return "fedcba0987654321fedcba0987654321";
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwarePackage::show(std::ostream& os)
{
    os << "SimSoftwarePackage:" << std::endl;
    os << "  Build: " << mBuildName << " v" << mBuildVersion << " (" << mBuildId << ")" << std::endl;
    os << "  Valid: " << (mValid ? "YES" : "NO") << std::endl;
    os << "  Active: " << (mActive ? "YES" : "NO") << std::endl;
    os << "  Running: " << (mRunning ? "YES" : "NO") << std::endl;
    os << "  Files:" << std::endl;
    for (const auto& file : mFiles) {
        os << "    - " << file << std::endl;
    }
}

//=============================================================================================================
// PRIVATE
//=============================================================================================================

//-------------------------------------------------------------------------------------------------------------
void SimSoftwarePackage::generateManifestPath()
{
    if (mFiles.empty()) {
        mManifestPath = "/tmp/manifest.json";
        return;
    }

    // Generate manifest path in same directory as first file
    std::string dir = Path::dir(mFiles[0]);
    mManifestPath = dir + "/manifest.json";
}

//-------------------------------------------------------------------------------------------------------------
void SimSoftwarePackage::extractBuildInfo()
{
    if (mFiles.empty()) {
        return;
    }

    // Try to extract version info from filename
    // Example: "ru-software-v1.2.3.bin" → version = "1.2.3"
    std::string filename = Path::basename(mFiles[0]);

    // Look for version pattern (v1.2.3 or similar)
    size_t vpos = filename.find("-v");
    if (vpos != std::string::npos) {
        size_t start = vpos + 2;
        size_t end = filename.find(".", start + 1);
        if (end != std::string::npos) {
            end = filename.find(".", end + 1);
            if (end != std::string::npos) {
                end = filename.find(".", end + 1);
                if (end != std::string::npos) {
                    mBuildVersion = filename.substr(start, end - start);
                }
            }
        }
    }

    // Use filename as build name
    mBuildName = filename;

    // Generate build ID from current timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "sim-" << time;
    mBuildId = ss.str();
}