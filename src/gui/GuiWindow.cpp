#include "GuiWindow.hpp"
#include <cstdio>
#include "Graphics.hpp"
#include "ControlRenderer.hpp"
#include "clap/AcidusClap.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

#if defined(__linux__) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace acidus {

GuiWindow::GuiWindow(AcidusClap* plugin)
    : plugin_(plugin), controlRenderer_(std::make_unique<TB303ControlRenderer>()) {
    pixelBuffer_.resize(width_ * height_, 0xFFDBDFE1);
    initControls();
}

GuiWindow::~GuiWindow() {
    destroy();
}

void GuiWindow::initControls() {
    controls_.clear();

    // Waveform selector leads the panel, rendered as a symbolic saw/square switch.
    controls_.push_back({ PARAM_WAVEFORM, "WAVEFORM", ControlType::ToggleSwitch, 56, 100, 17, 0.0, 1.0, 0.0, true });

    // 5 Main Knobs, labelled as on the TB-303 panel.
    controls_.push_back({ PARAM_CUTOFF, "CUT OFF FREQ", ControlType::Knob, 150, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_RESONANCE, "RESONANCE", ControlType::Knob, 240, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_ENV_MOD, "ENV MOD", ControlType::Knob, 330, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_DECAY, "DECAY", ControlType::Knob, 420, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_ACCENT, "ACCENT", ControlType::Knob, 510, 100, 20, 0.0, 1.0, 0.5, false });

    // Tuning trim, a real front-panel-equivalent control (TB303_RESEARCH_
    // COMPENDIUM.md documents a 50 kΩ "B" Tuning pot alongside Cutoff/Env
    // Mod/Decay/Accent) -- range matches the hardware's documented ±700
    // cent trim travel, for nudging the plugin into tune against a
    // reference recording that's itself slightly off-pitch.
    controls_.push_back({ PARAM_TUNE, "TUNING", ControlType::Knob, 610, 100, 20, -700.0, 700.0, 0.0, false });

    // MXR Distortion+ drive, set apart with a "hot" accent color. Fully
    // counter-clockwise (0.0) bypasses the pedal entirely.
    controls_.push_back({ PARAM_DRIVE, "DRIVE", ControlType::Knob, 710, 100, 20, 0.0, 1.0, 0.0, false });
    controls_.back().accentColor = 0xFF7A2418; // rust red, marks the distortion stage

    // Master Volume Knob, set apart with its own accent color.
    controls_.push_back({ PARAM_VOLUME, "VOLUME", ControlType::Knob, 810, 100, 20, 0.0, 1.0, 0.8, false });
    controls_.back().accentColor = 0xFF6B4A22; // warm amber, distinct from the graphite knobs

    updateKnobValuesFromPlugin();
}

void GuiWindow::updateKnobValuesFromPlugin() {
    if (!plugin_) return;
    for (size_t i = 0; i < controls_.size(); ++i) {
        if (static_cast<int>(i) == activeControlIndex_) continue;
        double val = 0.0;
        if (plugin_->paramsValue(controls_[i].id, &val)) {
            controls_[i].currentVal = val;
        }
    }
}

void GuiWindow::drawAcidusTitle(Graphics& g, int startX, int startY, unsigned parts) {
    // Helper lambda to draw letter primitives for ACIDUS
    auto renderLetters = [&](Graphics& gfx, int x, int y, uint32_t color) {
        // A
        int aX = x;
        gfx.drawRect(aX, y + 6, 5, 32, color);
        gfx.drawRect(aX + 15, y + 6, 5, 32, color);
        gfx.drawRect(aX + 4, y, 12, 6, color);
        gfx.drawRect(aX + 4, y + 18, 12, 5, color);

        // C
        int cX = x + 24;
        gfx.drawRect(cX, y, 5, 38, color);
        gfx.drawRect(cX, y, 18, 6, color);
        gfx.drawRect(cX, y + 32, 18, 6, color);

        // I
        int iX = x + 46;
        gfx.drawRect(iX, y, 10, 5, color);
        gfx.drawRect(iX + 3, y, 4, 38, color);
        gfx.drawRect(iX, y + 33, 10, 5, color);

        // D
        int dX = x + 60;
        gfx.drawRect(dX, y, 5, 38, color);
        gfx.drawRect(dX, y, 15, 6, color);
        gfx.drawRect(dX, y + 32, 15, 6, color);
        gfx.drawRect(dX + 15, y + 5, 5, 28, color);

        // U
        int uX = x + 84;
        gfx.drawRect(uX, y, 5, 34, color);
        gfx.drawRect(uX + 15, y, 5, 34, color);
        gfx.drawRect(uX, y + 32, 20, 6, color);

        // S
        int sX = x + 108;
        gfx.drawRect(sX, y, 20, 6, color);
        gfx.drawRect(sX, y, 5, 19, color);
        gfx.drawRect(sX, y + 16, 20, 6, color);
        gfx.drawRect(sX + 15, y + 18, 5, 17, color);
        gfx.drawRect(sX, y + 32, 20, 6, color);
    };

    // Glow layers (outer dim glow, inner medium glow, core acid green, core bright highlight)
    uint32_t dimGlow    = 0xFF0A400F; // Faint dark green outer halo
    uint32_t medGlow    = 0xFF1B8224; // Medium green inner halo
    uint32_t acidGreen  = 0xFF39FF14; // Core Acid Green
    uint32_t brightCore = 0xFFBFFF80; // Bright yellow-green center highlight

    // 0. Backing plate: a dark LCD-style badge the glow can pop against,
    // instead of sitting directly on the brushed silver panel.
    const int logoW = 128;   // A..S span, see renderLetters offsets above
    const int logoH = 38;
    const int subtitleGap = 8;
    const int subtitleH = 7; // font_ glyph height at scale 1
    int plateX = startX - 16;
    int plateY = startY - 14;
    int plateW = logoW + 32;
    const int smileyRow = 20;   // room for the acid smiley under the tagline
    const int presetRow = 14;   // calibration preset label under the smiley
    int plateH = logoH + subtitleGap + subtitleH + 26 + smileyRow + presetRow;
    logoPlateX_ = plateX;
    logoPlateY_ = plateY;
    logoPlateW_ = plateW;
    logoPlateH_ = plateH;

    const char* subtitle = "ANALOG BASS SYNTH";
    int subtitleW = font_.getTextWidth(subtitle, 1);
    int subtitleX = startX + (logoW - subtitleW) / 2;
    int subtitleY = startY + logoH + subtitleGap;

    if (parts & kTitlePlate) {
        g.fillRect(plateX + 3, plateY + 4, plateW, plateH, 0x50000000);
        g.fillRect(plateX, plateY, plateW, plateH, 0xFF131517);
        g.drawRect(plateX - 2, plateY - 2, plateW + 4, plateH + 4, 0xFF4A4E52);
        g.drawRect(plateX, plateY, plateW, plateH, 0xFF040506);
        g.drawLine(plateX + 2, plateY + 2, plateX + plateW - 3, plateY + 2, 0xFF2C3030, 1);
    }

    // (The bubbles are drawn here, between the plate and the lettering; see
    // renderFrame.)

    if (parts & kTitleForeground) {
        // Small corner screws for a hardware badge feel.
        for (int sx = 0; sx < 2; ++sx) {
            for (int sy = 0; sy < 2; ++sy) {
                int scx = plateX + 7 + sx * (plateW - 14);
                int scy = plateY + 7 + sy * (plateH - 14);
                g.fillCircle(scx, scy, 3, 0xFF3A3E42);
                g.drawCircle(scx, scy, 3, 0xFF08090A, 1);
                g.drawLine(scx - 2, scy, scx + 2, scy, 0xFF1A1C1E, 1);
            }
        }
    }

    if (parts & kTitleGlow) {
        // 1. Outer halo (pass offsets -2 to +2)
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                if (dx == 0 && dy == 0) continue;
                renderLetters(g, startX + dx, startY + dy, dimGlow);
            }
        }

        // 2. Inner halo (pass offsets -1 to +1)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                renderLetters(g, startX + dx, startY + dy, medGlow);
            }
        }
    }

    if (parts & kTitleForeground) {
        // 3. Core Acid Green
        renderLetters(g, startX, startY, acidGreen);

        // 4. Subtle inner highlight line
        renderLetters(g, startX + 1, startY + 1, brightCore);

        // 5. Tagline underneath the lockup, in the panel's own pixel font.
        g.drawText(font_, subtitle, subtitleX, subtitleY, 0xFF1B8224, 1);
    }

    if (!(parts & kTitleDynamic)) return;

    // 6. The acid smiley.
    drawSmiley(g, startX + logoW / 2, subtitleY + subtitleH + 13);

    // 7. Calibration preset label: a small recessed LCD window. A star
    // means a calibration parameter was changed after loading the preset.
    // Click to load the next preset.
    char label[48];
    if (plugin_) {
        std::snprintf(label, sizeof(label), "%s%s", plugin_->calibrationPresetName(),
                      plugin_->isCalibrationModified() ? "*" : "");
    } else {
        std::snprintf(label, sizeof(label), "%s", "X0X");
    }
    const int labelTextW = font_.getTextWidth(label, 1);
    presetLabelW_ = std::max(labelTextW, font_.getTextWidth("DEVIL FISH*", 1)) + 12;
    presetLabelH_ = 11;
    presetLabelX_ = startX + (logoW - presetLabelW_) / 2;
    presetLabelY_ = subtitleY + subtitleH + 27;
    g.fillRect(presetLabelX_, presetLabelY_, presetLabelW_, presetLabelH_, 0xFF070809);
    g.drawRect(presetLabelX_, presetLabelY_, presetLabelW_, presetLabelH_, 0xFF2C3030);
    g.drawLine(presetLabelX_ + 1, presetLabelY_ + 1, presetLabelX_ + presetLabelW_ - 2, presetLabelY_ + 1, 0xFF000000, 1);
    g.drawText(font_, label, presetLabelX_ + (presetLabelW_ - labelTextW) / 2, presetLabelY_ + 2, 0xFF39FF14, 1);
}

void GuiWindow::advanceAnimation(double dt) {
    if (!(dt > 0.0)) return;   // also rejects NaN
    dt = std::min(dt, 0.1);   // don't burst after a stall
    animTime_ += dt;
    eyeFlash_ = std::max(0.0, eyeFlash_ - dt);
    if (logoPlateW_ <= 0 || logoPlateH_ <= 0) return;

    // Bubble rate follows the Cutoff knob: a lazy trickle when closed, a
    // fizz when wide open.
    double cutoff = 0.5;
    for (const auto& c : controls_) {
        if (c.id == PARAM_CUTOFF) {
            const double norm = (c.currentVal - c.minVal) / (c.maxVal - c.minVal);
            cutoff = std::isfinite(norm) ? std::min(std::max(norm, 0.0), 1.0) : 0.5;
        }
    }
    const double rate = 0.5 + 12.0 * std::pow(cutoff, 1.5);   // bubbles per second
    bubbleSpawnAccum_ += rate * dt;

    std::uniform_real_distribution<double> uni(0.0, 1.0);
    while (bubbleSpawnAccum_ >= 1.0 && bubbles_.size() < 256) {
        bubbleSpawnAccum_ -= 1.0;
        Bubble b;
        b.r = 1.0 + 2.2 * uni(rng_);
        b.x = 6.0 + (logoPlateW_ - 12.0) * uni(rng_);
        b.y = logoPlateH_ - 4.0;
        b.speed = 10.0 + 18.0 * uni(rng_) + 6.0 / b.r;     // small ones rise faster
        b.phase = 6.283 * uni(rng_);
        bubbles_.push_back(b);
    }
    bubbleSpawnAccum_ = std::min(bubbleSpawnAccum_, 1.0);
    for (auto& b : bubbles_) {
        b.y -= b.speed * dt;
    }
    bubbles_.erase(std::remove_if(bubbles_.begin(), bubbles_.end(),
                                  [](const Bubble& b) { return b.y < 4.0 + b.r; }),
                   bubbles_.end());
}

void GuiWindow::drawBubbles(Graphics& g) {
    for (const auto& b : bubbles_) {
        double wobble = 1.6 * std::sin(animTime_ * 3.0 + b.phase + b.y * 0.08);
        int cx = logoPlateX_ + static_cast<int>(std::lround(b.x + wobble));
        int cy = logoPlateY_ + static_cast<int>(std::lround(b.y));
        int r = std::max(1, static_cast<int>(std::lround(b.r)));
        // Fade in near the bottom and out near the top of the plate.
        double t = std::min(1.0, std::min((logoPlateH_ - b.y) / 12.0, (b.y - 4.0) / 14.0));
        uint32_t a = static_cast<uint32_t>(std::max(0.0, t) * 0x90);
        g.fillCircle(cx, cy, r, (a / 3) << 24 | 0x0039FF14);
        g.drawCircle(cx, cy, r, a << 24 | 0x0039FF14, 1);
        if (r >= 2) g.fillCircle(cx - 1, cy - 1, 1, (a << 24) | 0x00CFFFB0);
    }
}

void GuiWindow::drawSmiley(Graphics& g, int cx, int cy) {
    const int r = 7;
    g.fillCircle(cx, cy, r + 2, 0x40FFD21E);   // soft glow on the dark plate
    g.fillCircle(cx, cy, r, 0xFFFFD21E);
    g.drawCircle(cx, cy, r, 0xFF5A4300, 1);
    if (eyeFlash_ > 0.0) {                          // eyes, wide on an accent
        g.fillRect(cx - 4, cy - 4, 3, 4, 0xFF101010);
        g.fillRect(cx + 2, cy - 4, 3, 4, 0xFF101010);
    } else {
        g.fillRect(cx - 3, cy - 3, 2, 3, 0xFF101010);
        g.fillRect(cx + 2, cy - 3, 2, 3, 0xFF101010);
    }
    for (int dx = -4; dx <= 4; ++dx) {               // smile
        int dy = static_cast<int>(std::lround(1.0 + 0.16 * (16 - dx * dx) * 0.28));
        g.fillRect(cx + dx, cy + 1 + dy, 1, 1, 0xFF101010);
    }
    g.fillRect(cx - 5, cy + 1, 1, 1, 0xFF101010);   // mouth corners
    g.fillRect(cx + 5, cy + 1, 1, 1, 0xFF101010);
}

double GuiWindow::glowPulse() const {
    // One slow breath per bar: brightest on the downbeat. Synced to the
    // host's bar while its transport plays, else a free-running 2 s cycle.
    double phase = 0.0;
    if (!plugin_ || !plugin_->transportBarPhase(phase)) {
        phase = std::fmod(animTime_ / 2.0, 1.0);
    }
    const double breath = 0.5 + 0.5 * std::cos(2.0 * 3.14159265358979323846 * phase);
    return 0.7 + 0.3 * breath;
}

void GuiWindow::renderFrame() {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    updateKnobValuesFromPlugin();

    // An accented note makes the smiley's eyes pop for a moment.
    if (plugin_) {
        const uint32_t accents = plugin_->accentCount();
        if (accents != lastAccentCount_) {
            lastAccentCount_ = accents;
            eyeFlash_ = kEyeFlashSec;
        }
    }

    auto now = std::chrono::steady_clock::now();
    if (haveLastFrameTime_ && !animationFrozen_) {
        advanceAnimation(std::chrono::duration<double>(now - lastFrameTime_).count());
    }
    lastFrameTime_ = now;
    haveLastFrameTime_ = true;

    const int hW = static_cast<int>(width_) * 2;
    const int hH = static_cast<int>(height_) * 2;
    const size_t hiResSize = static_cast<size_t>(hW) * static_cast<size_t>(hH);
    const int titleX = static_cast<int>(width_) - 180 + 26;
    const int titleY = 42;
    if (hiResBuffer_.size() != hiResSize) {
        hiResBuffer_.assign(hiResSize, 0);
        staticBuffer_.assign(hiResSize, 0);
        logoOverlay_.assign(hiResSize, 0);
        logoGlow_.assign(hiResSize, 0);
        staticKey_.clear();
        overlayValid_ = false;
    }

    // Only the logo plate animates. Everything else (panel, knobs, the
    // plate's box) is cached in staticBuffer_ and redrawn when a knob value
    // changes; the plate's lettering, screws and tagline are drawn once into
    // a transparent overlay. Each frame then repaints just the plate.
    std::vector<double> key;
    key.reserve(controls_.size());
    for (const auto& c : controls_) key.push_back(c.currentVal);
    const bool fullRedraw = (key != staticKey_);
    if (fullRedraw) {
        staticKey_ = key;
        Graphics g(staticBuffer_.data(), width_, height_, 2);

        // 1. Brushed silver panel background
        g.clear(0xFFDBDFE1);

        // Top & Bottom metallic borders / trims
        g.drawRect(0, 0, width_, 12, 0xFFC0C4C8);
        g.drawLine(0, 12, width_, 12, 0xFF808488, 1);
        g.drawLine(0, 13, width_, 13, 0xFFFFFFFF, 1);

        g.drawLine(0, height_ - 14, width_, height_ - 14, 0xFF808488, 1);
        g.drawRect(0, height_ - 13, width_, 13, 0xFFC0C4C8);

        // Vertical dividing line separating controls from right title panel
        int dividerX = static_cast<int>(width_) - 180;
        g.drawLine(dividerX, 14, dividerX, height_ - 14, 0xFF181818, 2);

        // 2. Draw Controls
        if (controlRenderer_) {
            for (const auto& ctrl : controls_) {
                if (ctrl.type == ControlType::Knob) {
                    controlRenderer_->drawKnob(g, ctrl, font_);
                } else if (ctrl.type == ControlType::ToggleSwitch) {
                    controlRenderer_->drawToggleSwitch(g, ctrl, font_);
                }
            }
        }
        drawAcidusTitle(g, titleX, titleY, kTitlePlate);
    }
    if (!overlayValid_) {
        std::fill(logoOverlay_.begin(), logoOverlay_.end(), 0u);
        Graphics og(logoOverlay_.data(), width_, height_, 2);
        drawAcidusTitle(og, titleX, titleY, kTitleForeground);
        std::fill(logoGlow_.begin(), logoGlow_.end(), 0u);
        Graphics gg(logoGlow_.data(), width_, height_, 2);
        drawAcidusTitle(gg, titleX, titleY, kTitleGlow);
        overlayValid_ = true;
    }

    // Region to repaint: the whole frame after a knob change, else the plate
    // (with its frame and shadow), in hi-res pixels.
    int rx0 = 0, ry0 = 0, rx1 = hW, ry1 = hH;
    if (!fullRedraw) {
        rx0 = std::max(0, 2 * (logoPlateX_ - 4));
        ry0 = std::max(0, 2 * (logoPlateY_ - 4));
        rx1 = std::min(hW, 2 * (logoPlateX_ + logoPlateW_ + 6));
        ry1 = std::min(hH, 2 * (logoPlateY_ + logoPlateH_ + 6));
    }
    for (int y = ry0; y < ry1; ++y) {
        std::memcpy(&hiResBuffer_[static_cast<size_t>(y) * hW + rx0],
                    &staticBuffer_[static_cast<size_t>(y) * hW + rx0],
                    static_cast<size_t>(rx1 - rx0) * sizeof(uint32_t));
    }

    Graphics g(hiResBuffer_.data(), width_, height_, 2);
    // 3. The logo plate: bubbles, then the cached lettering on top, then the
    // smiley and the calibration preset label.
    drawBubbles(g);
    const int ox0 = std::max(0, 2 * (logoPlateX_ - 4)), oy0 = std::max(0, 2 * (logoPlateY_ - 4));
    const int ox1 = std::min(hW, 2 * (logoPlateX_ + logoPlateW_ + 6)), oy1 = std::min(hH, 2 * (logoPlateY_ + logoPlateH_ + 6));
    // The lettering's halo breathes once per bar (see glowPulse), under the
    // lettering itself.
    const double pulse = glowPulse();
    const uint32_t glowAlpha = static_cast<uint32_t>(std::min(std::max(std::isfinite(pulse) ? 255.0 * pulse : 255.0, 0.0), 255.0));
    for (int y = oy0; y < oy1; ++y) {
        for (int x = ox0; x < ox1; ++x) {
            const size_t i = static_cast<size_t>(y) * hW + x;
            const uint32_t gl = logoGlow_[i];
            if (gl >> 24) g.blendPixel(x, y, (glowAlpha << 24) | (gl & 0x00FFFFFFu));
            const uint32_t o = logoOverlay_[i];
            if (o >> 24) g.blendPixel(x, y, o);
        }
    }
    drawAcidusTitle(g, titleX, titleY, kTitleDynamic);

    dirtyX_ = rx0 / 2; dirtyY_ = ry0 / 2; dirtyW_ = (rx1 - rx0) / 2; dirtyH_ = (ry1 - ry0) / 2;

    // 4. Downsample the repainted region (2x2 box filter) into pixelBuffer_
    pixelBuffer_.resize(width_ * height_);
    for (int py = ry0 / 2; py < ry1 / 2; ++py) {
        for (int px = rx0 / 2; px < rx1 / 2; ++px) {
            uint32_t p00 = hiResBuffer_[(2 * py) * hW + (2 * px)];
            uint32_t p01 = hiResBuffer_[(2 * py) * hW + (2 * px + 1)];
            uint32_t p10 = hiResBuffer_[(2 * py + 1) * hW + (2 * px)];
            uint32_t p11 = hiResBuffer_[(2 * py + 1) * hW + (2 * px + 1)];

            uint32_t r = (((p00 >> 16) & 0xFF) + ((p01 >> 16) & 0xFF) + ((p10 >> 16) & 0xFF) + ((p11 >> 16) & 0xFF) + 2) >> 2;
            uint32_t gVal = (((p00 >> 8) & 0xFF) + ((p01 >> 8) & 0xFF) + ((p10 >> 8) & 0xFF) + ((p11 >> 8) & 0xFF) + 2) >> 2;
            uint32_t b = ((p00 & 0xFF) + (p01 & 0xFF) + (p10 & 0xFF) + (p11 & 0xFF) + 2) >> 2;

            pixelBuffer_[py * width_ + px] = 0xFF000000 | (r << 16) | (gVal << 8) | b;
        }
    }

#if defined(__linux__) && !defined(__APPLE__)
    drawX11Frame();
#elif defined(_WIN32)
    drawWin32Frame();
#elif defined(__APPLE__)
    drawCocoaFrame();
#endif
}

void GuiWindow::handleMouseDown(int x, int y, bool isShift) {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    lastShiftState_ = isShift;

    // Click on the calibration label: load the next calibration preset.
    if (presetLabelW_ > 0 && x >= presetLabelX_ && x < presetLabelX_ + presetLabelW_
        && y >= presetLabelY_ && y < presetLabelY_ + presetLabelH_) {
        if (plugin_) {
            plugin_->cycleCalibrationPresetFromGui();
        }
        renderFrame();
        return;
    }

    for (size_t i = 0; i < controls_.size(); ++i) {
        auto& ctrl = controls_[i];
        if (ctrl.type == ControlType::Knob) {
            int dx = x - ctrl.x;
            int dy = y - ctrl.y;
            if (dx * dx + dy * dy <= (ctrl.radius + 10) * (ctrl.radius + 10)) {
                // Second click on the same knob within the window: reset it
                // to its default as one complete gesture, and don't start a drag.
                const auto now = std::chrono::steady_clock::now();
                const bool isDoubleClick = lastClickControl_ == static_cast<int>(i)
                                           && now - lastClickTime_ <= kDoubleClickWindow;
                lastClickControl_ = isDoubleClick ? -1 : static_cast<int>(i);
                lastClickTime_ = now;
                double defaultVal = 0.0;
                if (isDoubleClick && plugin_ && plugin_->paramsDefaultValue(ctrl.id, &defaultVal)) {
                    ctrl.currentVal = (std::min)((std::max)(defaultVal, ctrl.minVal), ctrl.maxVal);
                    plugin_->onBeginEditFromGui(ctrl.id);
                    plugin_->onParamValueFromGui(ctrl.id, ctrl.currentVal);
                    plugin_->onEndEditFromGui(ctrl.id);
                    renderFrame();
                    break;
                }
                activeControlIndex_ = static_cast<int>(i);
                dragStartY_ = y;
                dragStartVal_ = ctrl.currentVal;
                if (plugin_) {
                    plugin_->onBeginEditFromGui(ctrl.id);
                }
                break;
            }
        } else if (ctrl.type == ControlType::ToggleSwitch) {
            if (std::abs(x - ctrl.x) <= 22 && std::abs(y - ctrl.y) <= 40) {
                if (plugin_) {
                    plugin_->onBeginEditFromGui(ctrl.id);
                }
                double newVal;
                if (y < ctrl.y - 10) {
                    newVal = 0.0; // clicked the saw icon
                } else if (y > ctrl.y + 10) {
                    newVal = 1.0; // clicked the square icon
                } else {
                    newVal = (ctrl.currentVal >= 0.5) ? 0.0 : 1.0; // clicked the slider itself
                }
                ctrl.currentVal = newVal;
                if (plugin_) {
                    plugin_->onParamValueFromGui(ctrl.id, newVal);
                    plugin_->onEndEditFromGui(ctrl.id);
                }
                renderFrame();
                break;
            }
        }
    }
}

void GuiWindow::handleMouseDrag(int x, int y, bool isShift) {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    if (activeControlIndex_ < 0 || activeControlIndex_ >= static_cast<int>(controls_.size())) return;

    auto& ctrl = controls_[activeControlIndex_];
    if (ctrl.type != ControlType::Knob) return;

    if (isShift != lastShiftState_) {
        dragStartY_ = y;
        dragStartVal_ = ctrl.currentVal;
        lastShiftState_ = isShift;
    }

    int deltaY = dragStartY_ - y;
    if (deltaY != 0) lastClickControl_ = -1;   // a drag isn't the first half of a double-click

    double range = ctrl.maxVal - ctrl.minVal;
    if (!(range > 0.0)) return;
    double sensitivity = (isShift ? 0.0025 : 0.0125) * range;
    double newVal = dragStartVal_ + deltaY * sensitivity;

    newVal = (std::min)((std::max)(newVal, ctrl.minVal), ctrl.maxVal);
    ctrl.currentVal = newVal;

    if (plugin_) {
        plugin_->onParamValueFromGui(ctrl.id, newVal);
    }

    renderFrame();
}

void GuiWindow::handleMouseUp() {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    if (activeControlIndex_ >= 0 && activeControlIndex_ < static_cast<int>(controls_.size())) {
        if (plugin_) {
            plugin_->onEndEditFromGui(controls_[activeControlIndex_].id);
        }
    }
    activeControlIndex_ = -1;
}

bool GuiWindow::setParent(const clap_window_t* window) {
    if (!window || !window->api) return false;
#if defined(__linux__) && !defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_X11) == 0) {
        x11ParentWindow_ = window->x11;
        initX11Window();
        return true;
    }
#elif defined(_WIN32)
    if (std::strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0) {
        parentHwnd_ = window->win32;
        initWin32Window();
        return true;
    }
#elif defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0) {
        parentNsView_ = window->cocoa;
        initCocoaWindow();
        return true;
    }
#endif
    return false;
}

bool GuiWindow::setSize(uint32_t width, uint32_t height) {
    // The layout is fixed; refuse sizes that are degenerate or would allocate
    // absurd amounts (the 2x supersample buffer alone is 16 bytes/pixel).
    constexpr uint32_t kMaxDim = 4096;
    if (width == 0 || height == 0 || width > kMaxDim || height > kMaxDim) return false;
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    width_ = width;
    height_ = height;
    pixelBuffer_.assign(static_cast<size_t>(width_) * height_, 0xFFDBDFE1);
    renderFrame();
    return true;
}

bool GuiWindow::show() {
    visible_ = true;
    renderFrame();
    return true;
}

bool GuiWindow::hide() {
    visible_ = false;   // stops the animation repaints until shown again
    return true;
}

void GuiWindow::destroy() {
    isRunning_ = false;
    if (eventThread_.joinable()) {
        eventThread_.join();
    }
#if defined(__linux__) && !defined(__APPLE__)
    if (x11Display_ && x11Created_) {
        Display* display = static_cast<Display*>(x11Display_);
        XDestroyWindow(display, x11Window_);
        XCloseDisplay(display);
        x11Display_ = nullptr;
        x11Created_ = false;
    }
#endif
}

#if defined(__linux__) && !defined(__APPLE__)
void GuiWindow::initX11Window() {
    if (x11Created_) return;

    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    x11Display_ = display;
    int screen = DefaultScreen(display);
    Window parent = x11ParentWindow_ ? x11ParentWindow_ : RootWindow(display, screen);

    x11Window_ = XCreateSimpleWindow(display, parent, 0, 0, width_, height_, 0,
                                     BlackPixel(display, screen), WhitePixel(display, screen));

    XSelectInput(display, x11Window_, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(display, x11Window_);
    XFlush(display);

    x11Created_ = true;
    renderFrame();

    isRunning_ = true;
    eventThread_ = std::thread(&GuiWindow::eventLoopX11, this);
}

void GuiWindow::eventLoopX11() {
  try {
    if (!x11Display_) return;
    Display* display = static_cast<Display*>(x11Display_);
    auto lastX11Repaint = std::chrono::steady_clock::now();

    while (isRunning_) {
        while (XPending(display) > 0) {
            XEvent ev;
            XNextEvent(display, &ev);

            if (ev.type == Expose) {
                dirtyX_ = 0; dirtyY_ = 0;   // the server lost the window contents: send all
                dirtyW_ = static_cast<int>(width_); dirtyH_ = static_cast<int>(height_);
                drawX11Frame();
            } else if (ev.type == ButtonPress && ev.xbutton.button == Button1) {
                bool isShift = (ev.xbutton.state & ShiftMask) != 0;
                handleMouseDown(ev.xbutton.x, ev.xbutton.y, isShift);
            } else if (ev.type == MotionNotify) {
                if (ev.xmotion.state & Button1Mask) {
                    bool isShift = (ev.xmotion.state & ShiftMask) != 0;
                    handleMouseDrag(ev.xmotion.x, ev.xmotion.y, isShift);
                }
            } else if (ev.type == ButtonRelease && ev.xbutton.button == Button1) {
                handleMouseUp();
            }
        }
        // Periodic repaint for the logo plate's bubble animation (~30 fps).
        auto now = std::chrono::steady_clock::now();
        if (visible_ && now - lastX11Repaint >= std::chrono::milliseconds(33)) {
            lastX11Repaint = now;
            renderFrame();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  } catch (...) {
    // An exception escaping a thread function would std::terminate the host.
  }
}

void GuiWindow::drawX11Frame() {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    if (!x11Display_ || !x11Created_) return;
    if (pixelBuffer_.size() < static_cast<size_t>(width_) * height_) return;

    // Clip the dirty region to the image: an out-of-range XPutImage raises an
    // X protocol error, whose default handler exits the whole host process.
    const int imgW = static_cast<int>(width_), imgH = static_cast<int>(height_);
    int x0 = std::min(std::max(dirtyX_, 0), imgW), y0 = std::min(std::max(dirtyY_, 0), imgH);
    int x1 = std::min(std::max(dirtyX_ + dirtyW_, 0), imgW), y1 = std::min(std::max(dirtyY_ + dirtyH_, 0), imgH);
    if (x1 <= x0 || y1 <= y0) return;

    Display* display = static_cast<Display*>(x11Display_);
    int screen = DefaultScreen(display);

    XImage* image = XCreateImage(display, DefaultVisual(display, screen),
                                 24, ZPixmap, 0,
                                 reinterpret_cast<char*>(pixelBuffer_.data()),
                                 width_, height_, 32, 0);
    if (!image) return;

    GC gc = DefaultGC(display, screen);
    // Only the region renderFrame repainted (usually just the logo plate).
    XPutImage(display, x11Window_, gc, image, x0, y0, x0, y0,
              static_cast<unsigned>(x1 - x0), static_cast<unsigned>(y1 - y0));

    image->data = nullptr;
    XDestroyImage(image);
    XFlush(display);
}
#endif

#if defined(_WIN32)
static const wchar_t* kAcidusClassName = L"AcidusWindowCLASS";
static bool g_win32ClassRegistered = false;

static LRESULT CALLBACK AcidusWndProcImpl(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* gui = reinterpret_cast<GuiWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            gui = reinterpret_cast<GuiWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gui));
            SetTimer(hwnd, 1, 33, NULL);   // ~30 fps bubble animation
            return 0;
        }
        case WM_TIMER: {
            if (gui && gui->isVisible()) {
                gui->renderFrame();
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (gui) {
                gui->drawWin32Frame();
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (gui) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool isShift = (wParam & MK_SHIFT) != 0;
                SetCapture(hwnd);
                gui->handleMouseDown(x, y, isShift);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (gui && (wParam & MK_LBUTTON)) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool isShift = (wParam & MK_SHIFT) != 0;
                gui->handleMouseDrag(x, y, isShift);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (gui) {
                ReleaseCapture();
                gui->handleMouseUp();
            }
            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK AcidusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    try {
        return AcidusWndProcImpl(hwnd, msg, wParam, lParam);
    } catch (...) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);   // never unwind through the OS
    }
}

void GuiWindow::initWin32Window() {
    if (hwnd_) return;

    HINSTANCE hInstance = GetModuleHandleW(NULL);

    if (!g_win32ClassRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = AcidusWndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = kAcidusClassName;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        g_win32ClassRegistered = true;
    }

    HWND parent = static_cast<HWND>(parentHwnd_);

    hwnd_ = CreateWindowExW(
        0, kAcidusClassName, L"Acidus 303",
        WS_CHILD | WS_VISIBLE,
        0, 0, width_, height_,
        parent, NULL, hInstance, this
    );

    renderFrame();
}

void GuiWindow::drawWin32Frame() {
    std::lock_guard<std::recursive_mutex> lock(guiMutex_);
    if (!hwnd_) return;
    if (pixelBuffer_.size() < static_cast<size_t>(width_) * height_) return;

    HDC hdc = GetDC(static_cast<HWND>(hwnd_));
    if (!hdc) return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width_;
    bmi.bmiHeader.biHeight = -static_cast<int>(height_);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(
        hdc,
        0, 0, width_, height_,
        0, 0, 0, height_,
        pixelBuffer_.data(),
        &bmi,
        DIB_RGB_COLORS
    );

    ReleaseDC(static_cast<HWND>(hwnd_), hdc);
}
#endif

#if defined(__APPLE__)
void GuiWindow::initCocoaWindow() {}
void GuiWindow::drawCocoaFrame() {}
#endif

// CLAP GUI Extension Callbacks
const clap_plugin_gui_t g_acidusGuiExtension = {
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
#if defined(__linux__) && !defined(__APPLE__)
        return api && std::strcmp(api, CLAP_WINDOW_API_X11) == 0 && !is_floating;
#elif defined(_WIN32)
        return api && std::strcmp(api, CLAP_WINDOW_API_WIN32) == 0 && !is_floating;
#elif defined(__APPLE__)
        return api && std::strcmp(api, CLAP_WINDOW_API_COCOA) == 0 && !is_floating;
#else
        return false;
#endif
    },
    [](const clap_plugin_t* plugin, const char** api, bool* is_floating) -> bool {
        if (!api || !is_floating) return false;
#if defined(__linux__) && !defined(__APPLE__)
        *api = CLAP_WINDOW_API_X11;
#elif defined(_WIN32)
        *api = CLAP_WINDOW_API_WIN32;
#elif defined(__APPLE__)
        *api = CLAP_WINDOW_API_COCOA;
#endif
        *is_floating = false;
        return true;
    },
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        try { self->createGuiWindow(); } catch (...) { return false; }
        return true;
    },
    [](const clap_plugin_t* plugin) {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        try { self->destroyGuiWindow(); } catch (...) {}
    },
    [](const clap_plugin_t* plugin, double scale) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        if (!width || !height) return false;
        *width = 1070;
        *height = 180;
        return true;
    },
    [](const clap_plugin_t* plugin) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        if (!width || !height) return false;
        *width = 1070;
        *height = 180;
        return true;
    },
    [](const clap_plugin_t* plugin, uint32_t width, uint32_t height) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        try {
            if (self->getGuiWindow()) {
                return self->getGuiWindow()->setSize(width, height);
            }
        } catch (...) { return false; }
        return true;
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        if (!window || !window->api) return false;
        try {
            if (!self->getGuiWindow()) {
                self->createGuiWindow();
            }
            return self->getGuiWindow() && self->getGuiWindow()->setParent(window);
        } catch (...) { return false; }
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, const char* title) {},
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        try {
            if (self->getGuiWindow()) {
                return self->getGuiWindow()->show();
            }
        } catch (...) {}
        return false;
    },
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        try {
            if (self->getGuiWindow()) {
                return self->getGuiWindow()->hide();
            }
        } catch (...) {}
        return false;
    }
};

} // namespace acidus
