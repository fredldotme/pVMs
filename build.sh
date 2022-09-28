#/bin/bash

set -e
set -x

SRC_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SRC_PATH

if [ "$SNAPCRAFT_PART_INSTALL" != "" ]; then
    INSTALL=$SNAPCRAFT_PART_INSTALL
elif [ "$INSTALL_DIR" != "" ]; then
    INSTALL=$INSTALL_DIR
fi

if [ "$SNAPCRAFT_ARCH_TRIPLET" != "" ]; then
    ARCH_TRIPLET="$SNAPCRAFT_ARCH_TRIPLET"
fi

if [ -f /usr/bin/python3.8 ]; then
    PYTHON_BIN=/usr/bin/python3.8
elif [ -f /usr/bin/python3.6 ]; then
    PYTHON_BIN=/usr/bin/python3.6
fi

if [ "$PYTHON_BIN" == "" ]; then
    echo "PYTHON_BIN not found, bailing..."
fi

if [ "$INSTALL" == "" ]; then
    echo "Cannot find INSTALL, bailing..."
    exit 1
fi

# Argument variables
CLEAN=0
LEGACY=0

# Internal variables
if [ -f /usr/bin/dpkg-architecture ]; then
    MULTIARCH=$(/usr/bin/dpkg-architecture -qDEB_TARGET_MULTIARCH)
else
    MULTIARCH=""
fi

# pkg-config & m4 macros
PKG_CONF_SYSTEM=/usr/lib/$MULTIARCH/pkgconfig
PKG_CONF_INSTALL=$INSTALL/lib/pkgconfig:$INSTALL/share/pkgconfig:$INSTALL/lib/$MULTIARCH/pkgconfig
PKG_CONF_EXIST=$PKG_CONFIG_PATH
PKG_CONFIG_PATH=$PKG_CONF_INSTALL:$PKG_CONF_SYSTEM
if [ "$PKG_CONF_EXIST" != "" ]; then
    PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$PKG_CONF_EXIST"
fi
ACLOCAL_PATH=$INSTALL/share/aclocal

# Overridable number of build processors
if [ "$NUM_PROCS" == "" ]; then
    NUM_PROCS=$(nproc --all)
fi

# Argument parsing
while [[ $# -gt 0 ]]; do
    arg="$1"
    case $arg in
        -c|--clean)
            CLEAN=1
            shift
        ;;
        -l|--legacy)
            LEGACY=1
            shift
        ;;
        *)
            echo "usage: $0 [-c|--clean]"
            exit 1
        ;;
    esac
done

function build_3rdparty_autogen {
    echo "Building: $1"
    cd $SRC_PATH
    cd 3rdparty/$1
    if [ -f ./autogen.sh ]; then
        env PKG_CONFIG_PATH=$PKG_CONFIG_PATH ACLOCAL_PATH=$ACLOCAL_PATH ./autogen.sh --prefix=$INSTALL $2
    fi
    env PKG_CONFIG_PATH=$PKG_CONFIG_PATH ACLOCAL_PATH=$ACLOCAL_PATH ./configure --prefix=$INSTALL $2
    make VERBOSE=1 -j$NUM_PROCS
    if [ -f /usr/bin/sudo ]; then
        sudo make install
    else
        make install
    fi
}

function build_cmake {
    if [ "$CLEAN" == "1" ]; then
        if [ -d build ]; then
            rm -rf build
        fi
    fi
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    env PKG_CONFIG_PATH=$PKG_CONFIG_PATH LDFLAGS="-L$INSTALL/lib" \
        cmake .. \
        -DCMAKE_INSTALL_PREFIX=$INSTALL \
        -DCMAKE_MODULE_PATH=$INSTALL \
        -DCMAKE_CXX_FLAGS="-isystem $INSTALL/include -L$INSTALL/lib -Wno-deprecated-declarations -Wl,-rpath-link,$INSTALL/lib" \
        -DCMAKE_C_FLAGS="-isystem $INSTALL/include -L$INSTALL/lib -Wno-deprecated-declarations -Wl,-rpath-link,$INSTALL/lib" \
        -DCMAKE_LD_FLAGS="-L$INSTALL/lib" \
        -DCMAKE_LIBRARY_PATH=$INSTALL/lib $@
    make VERBOSE=1 -j$NUM_PROCS
    if [ -f /usr/bin/sudo ]; then
        sudo make install
    else
        make install
    fi
}

function build_3rdparty_cmake {
    echo "Building: $1"
    cd $SRC_PATH
    cd 3rdparty/$1
    build_cmake
}

function build_project {
    echo "Building project"
    cd $SRC_PATH
    cd src
    build_cmake $1
}

# Build direct dependencies
if [ ! -f $INSTALL/.xorg-macros_built ]; then
    build_3rdparty_autogen xorg-macros
    touch $INSTALL/.xorg-macros_built
fi

# Build direct dependencies
if [ ! -f $INSTALL/.libepoxy_built ]; then
    if [ -d $SRC_PATH/3rdparty/libepoxy/m4 ]; then
        rm -rf $SRC_PATH/3rdparty/libepoxy/m4
    fi
    build_3rdparty_autogen libepoxy "--enable-egl --disable-static --enable-shared --host=$ARCH_TRIPLET"
    touch $INSTALL/.libepoxy_built
fi

if [ ! -f $INSTALL/.virglrenderer_built ]; then
    build_3rdparty_autogen virglrenderer "--disable-static --enable-shared --enable-gbm-allocation --host=$ARCH_TRIPLET"
    touch $INSTALL/.virglrenderer_built
fi

if [ ! -f $INSTALL/.spice-protocol_built ]; then
    build_3rdparty_autogen spice-protocol
    touch $INSTALL/.spice-protocol_built
fi

if [ ! -f $INSTALL/.spice_built ]; then
    build_3rdparty_autogen spice "--disable-opus"
    touch $INSTALL/.spice_built
fi

if [ ! -f $INSTALL/.SDL_built ]; then
    build_3rdparty_cmake SDL
    touch $INSTALL/.SDL_built
fi

if [ ! -f $INSTALL/.qemu_built ]; then
    build_3rdparty_autogen qemu "--python=$PYTHON_BIN --audio-drv-list=pa --target-list=aarch64-softmmu,x86_64-softmmu --disable-strip --enable-virtiofsd --enable-opengl --enable-virglrenderer --enable-sdl --enable-spice --disable-werror"
    touch $INSTALL/.qemu_built
fi

# Attempt to strip binaries manually for improved file sizes
# Some files might be shell scripts so fail gracefully
for f in $(ls $INSTALL/bin/); do
    ${ARCH_TRIPLET}-strip $INSTALL/bin/$f || true
done

if [ "$LEGACY" == "1" ]; then
    LEGACY_ARG="-DPVMS_LEGACY=ON"
fi

# Build main sources
build_project "$LEGACY_ARG"
