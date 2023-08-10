# Required libraries:
# sudo apt-get install libsdl2-dev
# sudo apt-get install libsdl2-image-dev

if [[ -z "$1" ]]; then
    PRESET="linux-gcc-debug"
else
    PRESET="$1"
fi

if [[ -n "$2" ]]; then
    if [[ "$2" = "default" ]]; then
        GENERATOR="-G Unix Makefiles"
    else
        GENERATOR="-G $2"
    fi
fi

if [[ $0 == *fresh* ]]; then
    CONFIGURE_ARGS="${CONFIGURE_ARGS} --fresh"
fi

NPROC=$(getconf _NPROCESSORS_ONLN)

BUILD_DIR="./linux-build/${PRESET}"
echo CMake generate... && \
cmake -S . --preset ${PRESET} ${CONFIGURE_ARGS} ${GENERATOR:+"${GENERATOR}"} && \
echo CMake build... && \
cmake --build ${BUILD_DIR} -j ${NPROC} --verbose
