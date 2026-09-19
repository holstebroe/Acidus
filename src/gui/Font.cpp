#include "Font.hpp"
#include <cstring>

namespace acidus {

static uint8_t g_font5x7[128][7];
static bool g_fontInitialized = false;

static void initFontTable() {
    if (g_fontInitialized) return;
    std::memset(g_font5x7, 0, sizeof(g_font5x7));

    static const uint8_t f_0[7] = { 0x7C, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00 };
    static const uint8_t f_1[7] = { 0x30, 0x70, 0x30, 0x30, 0x30, 0xFC, 0x00 };
    static const uint8_t f_2[7] = { 0x78, 0xCC, 0x0C, 0x38, 0x60, 0xFC, 0x00 };
    static const uint8_t f_3[7] = { 0x78, 0xCC, 0x1C, 0x0C, 0xCC, 0x78, 0x00 };
    static const uint8_t f_4[7] = { 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x00 };
    static const uint8_t f_5[7] = { 0xFC, 0xC0, 0xF8, 0x0C, 0xCC, 0x78, 0x00 };
    static const uint8_t f_6[7] = { 0x38, 0x60, 0xC0, 0xF8, 0xCC, 0x78, 0x00 };
    static const uint8_t f_7[7] = { 0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x00 };
    static const uint8_t f_8[7] = { 0x78, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00 };
    static const uint8_t f_9[7] = { 0x78, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00 };

    static const uint8_t f_A[7] = { 0x30, 0x78, 0xCC, 0xFC, 0xCC, 0xCC, 0x00 };
    static const uint8_t f_B[7] = { 0xF8, 0xCC, 0xF8, 0xCC, 0xCC, 0xF8, 0x00 };
    static const uint8_t f_C[7] = { 0x78, 0xCC, 0xC0, 0xC0, 0xCC, 0x78, 0x00 };
    static const uint8_t f_D[7] = { 0xF0, 0x68, 0x6C, 0x6C, 0x68, 0xF0, 0x00 };
    static const uint8_t f_E[7] = { 0xFC, 0xC0, 0xF8, 0xC0, 0xC0, 0xFC, 0x00 };
    static const uint8_t f_F[7] = { 0xFC, 0xC0, 0xF8, 0xC0, 0xC0, 0xC0, 0x00 };
    static const uint8_t f_G[7] = { 0x78, 0xCC, 0xC0, 0xDC, 0xCC, 0x78, 0x00 };
    static const uint8_t f_H[7] = { 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00 };
    static const uint8_t f_I[7] = { 0x78, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00 };
    static const uint8_t f_J[7] = { 0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0x78, 0x00 };
    static const uint8_t f_K[7] = { 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xCC, 0x00 };
    static const uint8_t f_L[7] = { 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFC, 0x00 };
    static const uint8_t f_M[7] = { 0xCC, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00 };
    static const uint8_t f_N[7] = { 0xCC, 0xE6, 0xF6, 0xDE, 0xCE, 0xCC, 0x00 };
    static const uint8_t f_O[7] = { 0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x00 };
    static const uint8_t f_P[7] = { 0xF8, 0xCC, 0xCC, 0xF8, 0xC0, 0xC0, 0x00 };
    static const uint8_t f_Q[7] = { 0x78, 0xCC, 0xCC, 0xDC, 0x78, 0x1C, 0x00 };
    static const uint8_t f_R[7] = { 0xF8, 0xCC, 0xCC, 0xF8, 0xD8, 0xCC, 0x00 };
    static const uint8_t f_S[7] = { 0x78, 0xCC, 0x70, 0x1C, 0xCC, 0x78, 0x00 };
    static const uint8_t f_T[7] = { 0xFC, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00 };
    static const uint8_t f_U[7] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x00 };
    static const uint8_t f_V[7] = { 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00 };
    static const uint8_t f_W[7] = { 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xCC, 0x00 };
    static const uint8_t f_X[7] = { 0xCC, 0xCC, 0x78, 0x30, 0x78, 0xCC, 0x00 };
    static const uint8_t f_Y[7] = { 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x30, 0x00 };
    static const uint8_t f_Z[7] = { 0xFC, 0x0C, 0x18, 0x30, 0x60, 0xFC, 0x00 };

    static const uint8_t f_dash[7] = { 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t f_dot[7]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00 };

    std::memcpy(g_font5x7['0'], f_0, 7);
    std::memcpy(g_font5x7['1'], f_1, 7);
    std::memcpy(g_font5x7['2'], f_2, 7);
    std::memcpy(g_font5x7['3'], f_3, 7);
    std::memcpy(g_font5x7['4'], f_4, 7);
    std::memcpy(g_font5x7['5'], f_5, 7);
    std::memcpy(g_font5x7['6'], f_6, 7);
    std::memcpy(g_font5x7['7'], f_7, 7);
    std::memcpy(g_font5x7['8'], f_8, 7);
    std::memcpy(g_font5x7['9'], f_9, 7);

    std::memcpy(g_font5x7['A'], f_A, 7);
    std::memcpy(g_font5x7['B'], f_B, 7);
    std::memcpy(g_font5x7['C'], f_C, 7);
    std::memcpy(g_font5x7['D'], f_D, 7);
    std::memcpy(g_font5x7['E'], f_E, 7);
    std::memcpy(g_font5x7['F'], f_F, 7);
    std::memcpy(g_font5x7['G'], f_G, 7);
    std::memcpy(g_font5x7['H'], f_H, 7);
    std::memcpy(g_font5x7['I'], f_I, 7);
    std::memcpy(g_font5x7['J'], f_J, 7);
    std::memcpy(g_font5x7['K'], f_K, 7);
    std::memcpy(g_font5x7['L'], f_L, 7);
    std::memcpy(g_font5x7['M'], f_M, 7);
    std::memcpy(g_font5x7['N'], f_N, 7);
    std::memcpy(g_font5x7['O'], f_O, 7);
    std::memcpy(g_font5x7['P'], f_P, 7);
    std::memcpy(g_font5x7['Q'], f_Q, 7);
    std::memcpy(g_font5x7['R'], f_R, 7);
    std::memcpy(g_font5x7['S'], f_S, 7);
    std::memcpy(g_font5x7['T'], f_T, 7);
    std::memcpy(g_font5x7['U'], f_U, 7);
    std::memcpy(g_font5x7['V'], f_V, 7);
    std::memcpy(g_font5x7['W'], f_W, 7);
    std::memcpy(g_font5x7['X'], f_X, 7);
    std::memcpy(g_font5x7['Y'], f_Y, 7);
    std::memcpy(g_font5x7['Z'], f_Z, 7);

    std::memcpy(g_font5x7['-'], f_dash, 7);
    std::memcpy(g_font5x7['.'], f_dot, 7);

    g_fontInitialized = true;
}

Font::Font(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    initFontTable();
}

const uint8_t* Font::getGlyph(char c) const {
    uint8_t idx = static_cast<uint8_t>(c);
    if (idx < 128) {
        return g_font5x7[idx];
    }
    return g_font5x7[' '];
}

int Font::getTextWidth(const char* text, int scale) const {
    if (!text) return 0;
    size_t len = std::strlen(text);
    return static_cast<int>(len * (width_ + 1)) * scale;
}

Font Font::default5x7() {
    return Font(5, 7);
}

} // namespace acidus
