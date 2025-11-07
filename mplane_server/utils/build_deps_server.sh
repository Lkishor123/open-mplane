#!/bin/bash

# Display Help
display_help()
{
   echo "Usage: build_deps_server.sh [--dir <path>] [--help]"
   echo
   echo "options:"
   echo "--dir <path>      Specify the path to M-Plane server directory."
   echo "--help            Show this help message and exit."
   echo
}

# Parse options
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: no directory given for --dir"
                exit 1
            fi
            MPLANE_SERVER_DIR=$2
            shift 2
            ;;
        --help)
            display_help
            exit 0
            ;;
        *)
            echo "'$1' is not a valid options. See '--help'."
            exit 1
            ;;
    esac
done

if [[ -z "$MPLANE_SERVER_DIR" ]]; then
    if [[ $(git rev-parse --is-inside-work-tree 2>/dev/null) ]]; then
        # Get the absolute path of the root of this repo
        REPO_TOPLEVEL=$(git rev-parse --show-toplevel)
        MPLANE_SERVER_DIR=$REPO_TOPLEVEL/mplane_server
    else
        echo "Error: could not identify M-Plane server directory"
        exit 1
    fi
fi

if [[ ! -d "$MPLANE_SERVER_DIR/deps" ]]; then
    echo "Error: Dependencies not found in $MPLANE_SERVER_DIR/deps"
    echo "Please run get_deps_server.sh first"
    exit 1
fi

cd $MPLANE_SERVER_DIR/deps
mkdir -p install
mkdir -p install/bin
mkdir -p install/lib

# Symlink lib64 to lib because libyang/libnetconf2/sysrepo use lib and lib64
# depending on platform
if [[ ! -d install/lib64 ]]; then
    ln -s $(pwd)/install/lib install/lib64
fi

echo "Building libssh..."
mkdir -p libssh-0.9.2/build
cd libssh-0.9.2/build
cmake -D WITH_EXAMPLES=OFF \
      -D CMAKE_INSTALL_PREFIX:PATH='' \
      ..
make -j $(nproc)
make DESTDIR=$(pwd)/../../install install
cd ../..

echo "Building libyang..."
mkdir -p libyang/build
cd libyang/build
cmake -D ENABLE_BUILD_TESTS=OFF \
      -D GEN_LANGUAGE_BINDINGS=ON \
      -D GEN_CPP_BINDINGS=ON \
      -D CMAKE_INSTALL_PREFIX:PATH='' \
      ..
make -j $(nproc)
make DESTDIR=$(pwd)/../../install install
cd ../..

echo "Building libnetconf2..."
mkdir -p libnetconf2/build
cd libnetconf2/build
cmake -D LIBSSH_INCLUDE_DIR=../../install/include \
      -D LIBSSH_LIBRARY=../../install/lib64/libssh.so \
      -D LIBYANG_INCLUDE_DIR=../../install/include \
      -D LIBYANG_LIBRARY=../../install/lib64/libyang.so \
      -D ENABLE_SSH=ON \
      -D ENABLE_TLS=ON \
      -D ENABLE_BUILD_TESTS=OFF \
      -D CMAKE_INSTALL_PREFIX:PATH='' \
      ..
make -j $(nproc)
make DESTDIR=$(pwd)/../../install install
cd ../..

echo "Building sysrepo..."
mkdir -p sysrepo/build
cd sysrepo/build
cmake -D LIBYANG_INCLUDE_DIR=../../install/include \
      -D LIBYANG_LIBRARY=../../install/lib64/libyang.so \
      -D GEN_LANGUAGE_BINDINGS=ON \
      -D GEN_CPP_BINDINGS=ON \
      -D GEN_PYTHON_BINDINGS=OFF \
      -D ENABLE_PYTHON_TESTS=OFF \
      -D BUILD_EXAMPLES=OFF \
      -D CMAKE_BUILD_TYPE=Release \
      -D ENABLE_TESTS=OFF \
      -D CALL_TARGET_BINS_DIRECTLY=OFF \
      -D CMAKE_PREFIX_PATH=$(pwd)/../../install \
      -D CMAKE_INSTALL_PREFIX:PATH='' \
      ..
make -j $(nproc)
make DESTDIR=$(pwd)/../../install install
cd ../..

# Note: tinyxml2 is now handled via CMake FetchContent in mplane_server/CMakeLists.txt
# No manual build required here

echo "Building netopeer2..."
mkdir -p netopeer2/build
cd netopeer2/build
PATH=$(pwd)/../../install/bin:$PATH \
PKG_CONFIG_PATH=$(pwd)/../../install/lib/pkgconfig:$PKG_CONFIG_PATH \
cmake -D LIBSSH_INCLUDE_DIR=../../install/include \
      -D LIBSSH_LIBRARY=../../install/lib64/libssh.so \
      -D LIBYANG_INCLUDE_DIR=../../install/include \
      -D LIBYANG_LIBRARY=../../install/lib64/libyang.so \
      -D SYSREPO_INCLUDE_DIR=../../install/include \
      -D SYSREPO_LIBRARY=../../install/lib64/libsysrepo.so \
      -D LIBNETCONF2_INCLUDE_DIR=../../install/include \
      -D LIBNETCONF2_LIBRARY=../../install/lib64/libnetconf2.so \
      -D CMAKE_INSTALL_PREFIX:PATH='' \
      ..
make -j $(nproc)
PATH=$(pwd)/../../install/bin:$PATH LD_LIBRARY_PATH=$(pwd)/../../install/lib64:$LD_LIBRARY_PATH make DESTDIR=$(pwd)/../../install install
cd ../..

cd ..

echo "Dependencies built and installed to $MPLANE_SERVER_DIR/deps/install"
echo "Set CMAKE_PREFIX_PATH=\$MPLANE_SERVER_DIR/deps/install when building mplane_server"
