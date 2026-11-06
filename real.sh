cmake -B build -G Xcode
cmake --build build --config Release --target Pannrummet_All
cp -R build/Pannrummet_artefacts/Release/VST3/Pannrummet.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Pannrummet_artefacts/Release/AU/Pannrummet.component ~/Library/Audio/Plug-Ins/Components/
cp -R build/Pannrummet_artefacts/Release/CLAP/Pannrummet.clap ~/Library/Audio/Plug-Ins/CLAP/
