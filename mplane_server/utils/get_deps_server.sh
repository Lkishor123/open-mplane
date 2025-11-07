#!/bin/bash

# Enable the forward proxy unless explicitly told to disable
export https_proxy=fwdproxy:8080

# Display Help
display_help()
{
   echo "Usage: get_deps_server.sh [--no-fwdproxy] [--dir <path>] [--help]"
   echo
   echo "options:"
   echo "--no-fwdproxy     Disable HTTPS forward proxy."
   echo "--dir <path>      Specify the path to M-Plane server directory."
   echo "--help            Show this help message and exit."
   echo
}

# Parse options
script_name=$0
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --no-fwdproxy)
            unset https_proxy
            shift
            ;;
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

mkdir -p $MPLANE_SERVER_DIR/deps
cd $MPLANE_SERVER_DIR/deps

# Download dependencies needed for mplane_server
echo "Downloading libssh..."
wget -O libssh-0.9.2.tar.gz https://git.libssh.org/projects/libssh.git/snapshot/libssh-0.9.2.tar.gz
tar -xf libssh-0.9.2.tar.gz && rm libssh-0.9.2.tar.gz

echo "Downloading libyang..."
git clone --single-branch --branch v1.0.225 https://github.com/CESNET/libyang

echo "Downloading libnetconf2..."
git clone --single-branch --branch v1.1.43 https://github.com/CESNET/libnetconf2

echo "Downloading sysrepo..."
git clone https://github.com/sysrepo/sysrepo
cd sysrepo
git checkout 5b9b175ea3eac005bce1c13d24b09e56bfbdb55b
cd ..

echo "Downloading tinyxml2..."
git clone --single-branch --branch 10.0.0 https://github.com/leethomason/tinyxml2

echo "Downloading netopeer2..."
git clone --single-branch --branch v1.1.27 https://github.com/CESNET/netopeer2
cd ..

cd ..

echo "Dependencies downloaded to $MPLANE_SERVER_DIR/deps"