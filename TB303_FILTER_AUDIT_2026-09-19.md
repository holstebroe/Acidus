# TB-303 Filter Audit (2026-09-19)

Scope: `Filter.cpp` (Accurate + Faithful modes) and `Oscillator.cpp` as uploaded, measured in a standalone harness (44.1 kHz, 65.4 Hz saw through the real filter code), compared with a Python port of Open303's `TeeBeeFilter` TB_303 branch (source fetched from RobinSchmidt/Open303, `rosic_TeeBeeFilter.h`). Not available: `SynthEngine.cpp`, `Envelope.cpp` (VCA stage, accent, CV summing). Coupling helper classes (`inputCoupling_`, `outputCoupling_`) were stubbed as plain one-pole filters.

> **Status (2026-09-19, same day): applied.** Findings 1, 2, 5 (partially) and 6 below were independently re-derived from scratch (the characteristic polynomial and critical gain of exactly 17 were confirmed analytically and numerically, not just taken on trust) and implemented in `Filter.cpp`/`Filter.hpp`: the feedback gain ceiling is now capped at `kLadderCriticalGain_ * kResonanceGainMargin_` (17 × 0.90 ≈ 15.3) for both modes, the resonance knob is skewed via `skewResonance()` matching Open303's own taper, output-level compensation now rises to ~2.3x at max resonance, the capacitor pole-spread ratios were neutralized to 1.0 across all four stages, and the cutoff is now scaled by `1/sqrt(2)` so the labeled Hz value matches the actual resonance-peak frequency. `syrebas_filter_stability_test` confirms self-oscillation is eliminated across the full 0.0-1.0 Resonance range at every tested cutoff post-fix. See `TB303_PARAMETER_CONFIDENCE.md` for the up-to-date parameter table. The "open items" below (in particular, the specific ~2650 Hz render artifact that prompted this audit) were **not** resolved by this fix and remain open.

## Verdict

The midrange build-up is not caused by "nonlinearity in the ladder". It is caused by loop gain past the stability boundary. The tanh ladder is fine and worth keeping.

## Findings (ordered by impact)

1. **Feedback gain is ~2x past the self-oscillation threshold.** The ODE in `Filter.cpp` is the same linear ladder as Open303's (transposed): characteristic polynomial `s^4 + 8s^3 + 20s^2 + 16s + 2`, critical feedback gain `k = 17.0` exactly (Open303 uses 17 as its anchor). Shipped ceilings: Accurate 33, Faithful 36. With the 150-250 Hz feedback HPF the critical gain rises to ~19-38 depending on cutoff, so Accurate at res>=0.75 and Faithful at label cutoff >=1 kHz self-oscillate. Measured (65 Hz saw, static cutoff, energy in 800-3k Hz): Accurate 620 Hz/res 1 = 59%, 1 kHz/res 1 = 73%; Faithful 1 kHz/res 1 = 24%, 2 kHz/res 1 = 54%. This contradicts compendium §6/§12 and the confidence table's "consistent with no clean self-oscillation".
2. **The invented capScale ratios (1 / 0.667 / 0.303 / 1) distort the filter.** They halve the dominant pole (-0.152w -> -0.071w, -3 dB corner 0.149w -> 0.071w) and raise critical k to ~30-42, so kFb=36 was effectively tuned to compensate for them. They do not reproduce the "-0.13/-1.04/-2.33/-3.24" poles in the reference doc either (result: -0.07/-0.93/-2.32/-2.62).
3. **Reference doc §8 transfer function is not the linearisation of the §7 ODE the code implements.** Actual poles: -0.152, -1.235, -2.765, -3.848. Nobody checked internal consistency.
4. **Cutoff label is not calibrated to anything.** In Open303 the Hz label equals the resonance-peak frequency (measured ratio 1.01-1.10 at res 1, and it decays: no self-oscillation). Here omega = 2*pi*label, so the resonance peak sits at 1.2-2.0x the label and the -3 dB corner at 0.07-0.15x label. The 200 Hz-2.5 kHz "cutoff range" therefore means something different in each implementation.
5. **No resonance skew, no level compensation.** Open303: `r = (1-exp(-3x))/(1-exp(-3))` and output gain rises with resonance (about x2.3 at max). Here kFb is linear in the knob (knob 0.5 already = k 18 = critical) and level falls 13-16 dB at res 1 (Open303: ~8 dB).
6. **Open303 constants were transplanted without the calibration they depend on.** 150 Hz feedback HPF and 44.5 Hz osc HPF were copied; the k ceiling, resonance skew, output gain compensation and cutoff-to-b0 fit that make them work were not. Do not copy Open303's k(fx) polynomial either: it compensates its own explicit (Euler-style) discretisation. For the trapezoid/Newton solver here the continuous-time critical gain applies.

## Evidence that nonlinearity is not the culprit

- Faithful, equal caps, kFb 17 vs 36 (cutoff env 250->620->250, 65 Hz saw, res 1.0, first 250 ms): 800-3k energy 0.2% vs 11.1%; 150-800 energy 8.6% vs 51.7%.
- A "corrected" tanh ladder (equal caps, omega x0.697, k ceiling ~17 x skewed resonance, Open303-style gain comp) tracks Open303's level within ~2-3 dB and spectral shape at label 620 and 1000 Hz, and is still amplitude dependent: at label 1 kHz/res 0.75, input x0.25 -> x4 shifts 800-3k energy from 6.4% to 0.8%. A linear Open303-style filter cannot do that. That amplitude-dependent behaviour is the route to going beyond Open303.
- At label 2 kHz the corrected variant still self-oscillated when it borrowed Open303's k polynomial (k~24) -> confirms point 6.

## Open items / not verified

- The reported 93% mid-band share could not be reproduced from the filter alone; a post-filter cause (VCA gain/tanh, accent, CV summing) is not ruled out without `SynthEngine.cpp` / `Envelope.cpp`. Discriminator: render the same note with Resonance = 0. If mid energy persists, look downstream of the filter.
- Oscillator is a naive (non band-limited) saw at host rate; harmless at bass pitches, worth oversampling later.
- Only Open303's TB_303 filter branch was verified from source; claims about the rest of its chain (VCA, shaper) are unverified.

## Suggested order for the fixes (as originally written; see status note above for what was applied)

1. Cap feedback at the true critical gain with margin.
2. Add a skewed resonance knob plus output-level compensation.
3. Remove the capScale ratios.
4. Define the cutoff as the resonance-peak frequency (an omega scale of about 0.7).

## Sources

- [rosic_TeeBeeFilter.h (Open303)](https://github.com/RobinSchmidt/Open303/blob/master/Source/DSPCode/rosic_TeeBeeFilter.h)
- [rosic_TeeBeeFilter.cpp (Open303)](https://github.com/RobinSchmidt/Open303/blob/master/Source/DSPCode/rosic_TeeBeeFilter.cpp)
