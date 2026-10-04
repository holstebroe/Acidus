# clap-wrapper patches

VST3 MIDI output fixes for clap-wrapper v0.16.0 (the `clap-wrapper`
submodule), kept here until they live on a fork and the submodule points
at it. See https://github.com/free-audio/clap-wrapper/issues/414.

1. `0001`: the VST3 wrapper passes a plugin's output MIDI note on/off,
   MIDI poly pressure and pressure note expressions to the host. Without
   it, `burette.vst3` cannot send the accent of a same-pitch slide
   (TB303_REFERENCE.md §4.6).
2. `0002`: incoming VST3 poly pressure with note id -1 is addressed by
   its pitch, as VST3 defines.

Apply to the submodule with `git -C clap-wrapper am ../patches/clap-wrapper/*.patch`.
