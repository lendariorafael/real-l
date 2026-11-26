# Real Lab

Plugin VST3 para Windows (FL Studio 26) inspirado no FLEX: sampler e sintetizador com biblioteca de timbres, knobs, faders, botões, presets e efeitos (Filter, Vibrato, Gated, Character, Reverb, Delay).

## Como obter o plugin (sem instalar nada no PC)

1. Faz commit deste projeto no teu repositório GitHub.
2. No separador **Actions**, abre a execução "Build Real Lab VST3" (corre automaticamente em cada push, ou via *Run workflow*).
3. No fim da execução, descarrega o artefacto **RealLab-VST3-Windows** (contém `Real Lab.vst3`).

## Instalação

1. Copia a pasta `Real Lab.vst3` para `C:\Program Files\Common Files\VST3\`.
2. Abre o FL Studio, vai a *Options > Manage plugins* e clica em *Find more plugins* (ou *Verify*).
3. Carrega o Real Lab no Channel Rack ou no Mixer.

## Samples e timbres próprios

- Clica em **Load Samples** e escolhe um ficheiro (WAV/AIFF/FLAC/OGG/MP3) ou uma pasta.
  - Um ficheiro: é tocado em todo o teclado, com a nota de origem no C4.
  - Uma pasta: os ficheiros, por ordem alfabética, são distribuídos entre C2 e C6.
- Quando há samples carregadas, o Real Lab usa o sampler; caso contrário usa o sintetizador interno.
- **Clear** volta ao sintetizador.

## Presets

- A coluna **LIBRARY** mostra os presets de fábrica e os teus presets.
- **Save Preset** grava o estado atual como ficheiro XML.
- Os presets ficam em `%APPDATA%\RealLab\Presets`. Podes copiar para lá ficheiros `.xml` de outras máquinas e aparecem na lista depois de guardar outro preset (ou reabrir o plugin).

## Parâmetros

| Secção      | Controlo                                   |
|-------------|--------------------------------------------|
| Source      | Wave (Saw/Square/Triangle/Sine), Unison    |
| Envelope    | Attack, Release                            |
| Filter      | Filter (cutoff), Resonance                 |
| Master      | Volume, Bypass FX                          |
| Modulation  | Vib Rate, Vibrato, Gate Rate, Gated        |
| Character   | Character (saturação)                      |
| Reverb      | Reverb (mix), Room                         |
| Delay       | Time, Feedback, Delay (mix)                |

O teclado em baixo toca com o rato. No FL Studio, o plugin também responde a MIDI.

## Compilar localmente (opcional)

Requer CMake 3.22+, Visual Studio 2022 com "Desenvolvimento para desktop com C++" e Git.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Licença

Usa o framework JUCE (versão 8.0.4), que tem licença GPLv3 ou comercial. Para uso pessoal, a versão gratuita serve; para distribuir comercialmente, verifica os termos da JUCE.
