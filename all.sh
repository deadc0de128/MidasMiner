PRESETS=$(cmake -S . --list-presets | grep -E '^.*\".*\".*-.*$' | sed -E 's/^.*\"(.*)\".*$/\1/')

for preset in ${PRESETS}; do
    ./build.sh ${preset}
done
