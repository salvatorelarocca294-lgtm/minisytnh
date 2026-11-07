# MiniSynth (VST3)

Synth polifonico a 8 voci: 2 oscillatori (sine/saw/square/triangle, polyBLEP), detune, filtro passa-basso con risonanza e inviluppo, ADSR, gain.

## Come ottenere il file .vst3 per Windows (senza installare nulla)
1. Crea un repository su GitHub e carica tutto il contenuto di questa cartella (compresa `.github`).
2. Vai nella scheda **Actions**, apri l'ultima esecuzione "Build VST3 (Windows)" e scarica l'artifact **MiniSynth-VST3-Windows**.
3. Copia la cartella `MiniSynth.vst3` in `C:\Program Files\Common Files\VST3\`.

## Compilare in locale (Windows)
Servono Visual Studio 2022 (workload C++) e CMake:
    cmake -B build
    cmake --build build --config Release
Risultato: `build\MiniSynth_artefacts\Release\VST3\MiniSynth.vst3`
