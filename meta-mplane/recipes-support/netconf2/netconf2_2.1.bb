SUMMARY = "libnetconf2"
DESCRIPTION = ""
SECTION = "core"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=08a5578c9bab06fb2ae84284630b973f"

SRC_URI = "git://github.com/CESNET/libnetconf2.git;protocol=https;branch=v2"

SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"

inherit cmake pkgconfig

DEPENDS = "openssl yang libssh libxcrypt"

OECMAKE_EXTRA_append = " -DCMAKE_INSTALL_PREFIX:PATH=/usr"
OECMAKE_EXTRA_append = " -DENABLE_TLS=ON"
OECMAKE_EXTRA_append = " -DENABLE_SSH=ON"
