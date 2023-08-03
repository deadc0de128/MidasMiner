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
cmake -S . --preset ${PRESET} ${CONFIGURE_ARGS} ${GENERATOR:+"${GENERATOR}"}
RET=$?; if [[ ${RET} -ne 0 ]]; then exit ${RET}; fi
cmake --build ${BUILD_DIR} -j ${NPROC} --verbose
RET=$?; if [[ ${RET} -ne 0 ]]; then exit ${RET}; fi
