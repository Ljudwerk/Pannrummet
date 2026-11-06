# Utvecklingsbygge: fristående app (arm64, Debug), testfallen och panelbilden.
cmake -B build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-dev --target Pannrummet_Standalone PRSnapshot PRRender
./build-dev/PRRender_artefacts/Debug/PRRender --test
./build-dev/PRSnapshot_artefacts/Debug/PRSnapshot docs/panelen.png 2
open build-dev/Pannrummet_artefacts/Debug/Standalone/Pannrummet.app
