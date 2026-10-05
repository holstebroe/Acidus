#ifndef ACIDUS_SEQ_SEQUENCER_LAYOUT_HPP
#define ACIDUS_SEQ_SEQUENCER_LAYOUT_HPP

// Burette's window layout, shared by the input mapping (SequencerGui.cpp)
// and the skins that draw it (RetroSequencerSkin.cpp,
// ModernSequencerSkin.cpp).

namespace acidus {
namespace seq {
namespace layout {

constexpr int kGridX = 100;
constexpr int kCellW = 48;
constexpr int kTitleY = 10, kTitleH = 26;
constexpr int kPatternY = 46, kPatternH = 30;
constexpr int kGridY = 88;
constexpr int kRowH[4] = { 52, 36, 36, 36 };
constexpr int kSetupY = 258, kSetupH = 26;
constexpr int kFooterY = 298;
constexpr int kHintX = 736, kHintValueX = 814;

inline int rowTop(int row) {
    int y = kGridY;
    for (int r = 0; r < row; ++r) y += kRowH[r];
    return y;
}

} // namespace layout
} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_LAYOUT_HPP
