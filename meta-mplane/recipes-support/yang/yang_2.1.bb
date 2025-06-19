DESCRIPTION = "Yang library"
SECTION = "core"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2982272c97a8e417a844857ca0d303b1"

FILESEXTRAPATHS_append := ":${META_MPLANE_DIR}/recipes-support/yang/files"

SRC_URI = "git://github.com/CESNET/libyang.git;protocol=https;branch=v2.1.x"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS = "libpcre"
RDEPENDS_${PN} = "libpcre"

EXTRA_OECMAKE_append = " -DGEN_LANGUAGE_BINDINGS=1"
EXTRA_OECMAKE_append = " -DGEN_CPP_BINDINGS=1"

FILES_${PN}-dev = "${libdir}/libyang.so \
                   ${libdir}/libyang-cpp.so \
                   ${libdir}/libyang.so.2 \
                   ${bindir} \
                  "
FILES_${PN} = "${libdir} \
               ${includedir} \
              "
