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

if [ "$BUILD_DIR" == "" ]; then
    BUILD_DIR="$INSTALL"
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
    if [ ! -f "$BUILD_DIR/.${1}_built" ]; then
        if [ -f ./autogen.sh ]; then
            env PKG_CONFIG_PATH=$PKG_CONFIG_PATH ACLOCAL_PATH=$ACLOCAL_PATH LD_LIBRARY_PATH=$INSTALL/lib:$LD_LIBRARY_PATH ./autogen.sh --prefix=$INSTALL $2
        fi
        env PKG_CONFIG_PATH=$PKG_CONFIG_PATH ACLOCAL_PATH=$ACLOCAL_PATH LD_LIBRARY_PATH=$INSTALL/lib:$LD_LIBRARY_PATH ./configure --prefix=$INSTALL $2
        make VERBOSE=1 -j$NUM_PROCS
    fi
    if [ -f /usr/bin/sudo ]; then
        sudo make install -j$NUM_PROCS
    else
        make install -j$NUM_PROCS
    fi
    touch $BUILD_DIR/.${1}_built
}

function build_3rdparty_meson {
    echo "Building: $1"
    cd $SRC_PATH
    cd 3rdparty/$1
    if [ "$CLEAN" == "1" ]; then
        if [ -d build ]; then
            rm -rf build
        fi
    fi
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    if [ ! -f "$BUILD_DIR/.${1}_built" ]; then
        meson $2 ..
        ninja -j$NUM_PROCS
    fi
    if [ -f /usr/bin/sudo ]; then   
        sudo ninja install
    else
        ninja install
    fi
    touch $BUILD_DIR/.${1}_built
}

function build_3rdparty_qmake {
    echo "Building: $1"
    cd $SRC_PATH
    cd 3rdparty/$1
    if [ "$CLEAN" == "1" ]; then
        if [ -d build ]; then
            rm -rf build
        fi
    fi
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    if [ ! -f "$BUILD_DIR/.${1}_built" ]; then
        env PKG_CONFIG_PATH=$PKG_CONFIG_PATH LD_LIBRARY_PATH=$INSTALL/lib:$LD_LIBRARY_PATH LDFLAGS="-L$INSTALL/lib" \
        qmake .. $2
        qmake -set prefix $INSTALL
        make VERBOSE=1 -j$NUM_PROCS
    fi
    
    if [ -f /usr/bin/sudo ]; then
        sudo make install -j$NUM_PROCS
    else
        make install -j$NUM_PROCS
    fi
    
    touch $BUILD_DIR/.${1}_built
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
    if [ ! -f "$BUILD_DIR/.${1}_built" ]; then
        env PKG_CONFIG_PATH=$PKG_CONFIG_PATH LD_LIBRARY_PATH=$INSTALL/lib:$LD_LIBRARY_PATH LDFLAGS="-L$INSTALL/lib" \
            cmake .. \
            -DCMAKE_INSTALL_PREFIX=$INSTALL \
            -DCMAKE_MODULE_PATH=$INSTALL \
            -DCMAKE_CXX_FLAGS="-isystem $INSTALL/include -L$INSTALL/lib -Wno-deprecated-declarations -Wl,-rpath-link,$INSTALL/lib" \
            -DCMAKE_C_FLAGS="-isystem $INSTALL/include -L$INSTALL/lib -Wno-deprecated-declarations -Wl,-rpath-link,$INSTALL/lib" \
            -DCMAKE_LD_FLAGS="-L$INSTALL/lib" \
            -DCMAKE_LIBRARY_PATH=$INSTALL/lib $@
        make VERBOSE=1 -j$NUM_PROCS
    fi

    if [ -f /usr/bin/sudo ]; then
        sudo make install -j$NUM_PROCS
    else
        make install -j$NUM_PROCS
    fi

    touch $BUILD_DIR/.${1}_built
}

function build_3rdparty_cmake {
    echo "Building: $1"
    cd $SRC_PATH
    cd 3rdparty/$1
    build_cmake "$2"
}

function build_project {
    echo "Building project"
    cd $SRC_PATH
    cd src
    build_cmake $1
}


# Use ccache as much as possible
export CCACHE_DIR=$BUILD_DIR/ccache
export PATH=/usr/lib/ccache:$PATH

# Mirclient enablement
MIRCLIENT_GTK=""
MIRCLIENT_SDL=""
#if [ "$LEGACY" == "0" ]; then
#    MIRCLIENT_GTK="--enable-mir-backend"
#    MIRCLIENT_SDL="--enable-video-mir --disable-mir-shared"
#fi


# Build direct dependencies
build_3rdparty_autogen xorg-macros
if [ ! -f "$BUILD_DIR/.libepoxy_built" ] && [ -d $SRC_PATH/3rdparty/libepoxy/m4 ]; then
    rm -rf $SRC_PATH/3rdparty/libepoxy/m4
fi
build_3rdparty_autogen libepoxy "--enable-egl=yes --enable-glx=yes --disable-static --enable-shared --host=$ARCH_TRIPLET"
build_3rdparty_meson virglrenderer "-Dvideo=true -Dprefix=$INSTALL"
build_3rdparty_autogen wayland-protocols "--host=$ARCH_TRIPLET"
#build_3rdparty_qmake qmltermwidget
#build_3rdparty_autogen glib "--host=$ARCH_TRIPLET --disable-gtk-doc --disable-installed-tests"
#build_3rdparty_autogen gtk "--disable-x11-backend --enable-wayland-backend $MIRCLIENT_GTK \
#        --disable-installed-tests --disable-gtk-doc \
#        --host=$ARCH_TRIPLET"
build_3rdparty_autogen SDL "--disable-video-x11 --enable-video-wayland --disable-wayland-shared \
        $MIRCLIENT_SDL \
        --enable-video-opengles  --disable-video-opengl --disable-video-vulkan \
        --disable-alsa-shared --disable-pulseaudio-shared \
        --enable-pulseaudio --enable-hidapi --enable-libudev --enable-dbus --disable-static --host=$ARCH_TRIPLET"
build_3rdparty_autogen qemu "--python=$PYTHON_BIN \
        --audio-drv-list=pa --target-list=aarch64-softmmu,x86_64-softmmu \
        --disable-strip --enable-virtiofsd --enable-opengl --enable-virglrenderer --enable-slirp \
        --enable-sdl --disable-spice --disable-werror --cross-prefix=$ARCH_TRIPLET-"

# Attempt to strip binaries manually for improved file sizes
# Some files might be shell scripts so fail gracefully
#for f in $(ls $INSTALL/bin/); do
#    ${ARCH_TRIPLET}-strip $INSTALL/bin/$f || true
#done

if [ "$LEGACY" == "1" ]; then
    LEGACY_ARG="-DPVMS_LEGACY=ON"
fi

# Download newer builds from EDK2
#if [ -d $INSTALL/efi ]; then
#    rm -rf $INSTALL/efi
#fi
#mkdir $INSTALL/efi
#mkdir $INSTALL/efi/aarch64
#mkdir $INSTALL/efi/x86_64
#wget -O $INSTALL/efi/x86_64/code.fd https://github.com/retrage/edk2-nightly/raw/master/bin/RELEASEX64_OVMF_CODE.fd # --no-check-certificate
#wget -O $INSTALL/efi/aarch64/code.fd https://github.com/retrage/edk2-nightly/raw/master/bin/RELEASEAARCH64_QEMU_EFI.fd # --no-check-certificate

# Build main sources
build_project "$LEGACY_ARG"
