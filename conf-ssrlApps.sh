#!/usr/bin/env bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")"

function usage {
    echo "$0 -t ARCH -p PREFIX [-b BSPS] [-k PATCH]"
    echo "WHERE:"
    echo "  -t ARCH      - Valid ARCH: powerpc i386 or m68k"
    echo "  -p PREFIX    - Top of the RTEMS install. Set to \$PWD/../.. by default"
    echo "  -b BSPS      - Comma separated list of BSPs. Defaults determined by arch setting"
    echo "  -k PATCH     - Patch level. i.e. rtems_p4. Set to 'rtems' by default"
}

while test $# -gt 0; do
    case $1 in
    -p)
        PREFIX="$2"
        shift 2
        ;;
    -t)
        TARGET="$2"
        shift 2
        ;;
    -b)
        BSPS="$2"
        shift 2
        ;;
    -k)
        RTEMS_PATCH="$2"
        shift 2
        ;;
    *)
        usage
        exit 1
        ;;
    esac
done

if [ -z "$PREFIX" ]; then
    PREFIX="$PWD/../.."
fi

if [ -z "$RTEMS_PATCH" ]; then
    RTEMS_PATCH=rtems
fi

if [ -z "$BSPS" ]; then
    case $TARGET in
    powerpc)
        BSPS="mvme3100 mvme6100"
        ;;
    m68k)
        BSPS="uC5282"
        ;;
    i386)
        BSPS="pc686"
        ;;
    *)
        ;;
    esac
fi

case $TARGET in
    powerpc|m68k|i386)
        ;;
    *)
        echo "Invalid target arch!"
        echo "Must be one of: powerpc m68k i386" 
        exit 1
        ;;
esac

if [ ! -f configure ]; then
    ./bootstrap
fi

mkdir -p build/$TARGET-rtems
cd build/$TARGET-rtems

../../configure \
    --prefix="$PREFIX" \
    --enable-rtemsbsp="$BSPS" \
    --with-rtems-top="$PREFIX/target/$RTEMS_PATCH" \
    --with-package-subdir="target/$RTEMS_PATCH/ssrlApps" \
    --exec-prefix="$PREFIX/host/linux-x86_64"