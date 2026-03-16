// GlitchClock - stateless rendering & data tables
#pragma once
#include <cstdint>
#include "esphome/core/color.h"
#include "esphome/components/light/addressable_light.h"

namespace gc {

// ── 7-segment digit bitmasks (bit order: gfedcba) ──────────────
//     _a_
//    |   |     bit 0 = a, bit 1 = b, ... bit 5 = f, bit 6 = g
//    f   b
//    |_g_|
//    |   |
//    e   c
//    |_d_|
const uint8_t DIGITS[] = {
  0x3F,  // 0: abcdef
  0x06,  // 1: bc
  0x5B,  // 2: abdeg
  0x4F,  // 3: abcdg
  0x66,  // 4: bcfg
  0x6D,  // 5: acdfg
  0x7D,  // 6: acdefg
  0x07,  // 7: abc
  0x7F,  // 8: abcdefg
  0x6F,  // 9: abcdfg
};

// Physical strip position → bit index in DIGITS bitmask
// Strip wiring: g(0), e(1), d(2), c(3), b(4), a(5), f(6)
const uint8_t SEG_MAP[] = {6, 4, 3, 2, 1, 0, 5};

// Outer segments in clockwise order (physical positions) for spinner
// Visual: a→b→c→d→e→f = physical positions 5→4→3→2→1→6
const uint8_t SPIN[] = {5, 4, 3, 2, 1, 6};

// ── 7-segment letter bitmasks (bit order: gfedcba) ─────────────
const uint8_t L_A = 0x77, L_b = 0x7C, L_C = 0x39, L_d = 0x5E;
const uint8_t L_E = 0x79, L_F = 0x71, L_G = 0x3D, L_H = 0x76;
const uint8_t L_I = 0x06, L_J = 0x1E, L_L = 0x38, L_n = 0x54;
const uint8_t L_o = 0x5C, L_P = 0x73, L_r = 0x50, L_S = 0x6D;
const uint8_t L_t = 0x78, L_u = 0x1C, L_U = 0x3E, L_y = 0x6E;

// ── 420 detection ────────────────────────────────────────────
inline bool is_420_time(int hour, int minute) {
  return (hour == 4 || hour == 16) && minute == 20;
}

// Easter egg words (raw segment bitmasks, 4 digits each)
const uint8_t EGGS[][4] = {
  {L_d, L_E, L_A, L_d},  // dEAd
  {L_b, L_E, L_E, L_F},  // bEEF
  {L_F, L_o, L_o, L_d},  // Food
  {L_C, L_A, L_F, L_E},  // CAFE
  {L_F, L_E, L_E, L_d},  // FEEd
  {L_H, L_E, L_L, L_P},  // HELP
  {L_H, L_E, L_L, L_L},  // HELL
  {L_C, L_o, L_d, L_E},  // CodE
  {L_F, L_A, L_d, L_E},  // FAdE
  {L_b, L_A, L_b, L_E},  // bAbE
  {L_F, L_A, L_C, L_E},  // FACE
  {L_C, L_o, L_o, L_L},  // CooL
  {L_F, L_E, L_A, L_r},  // FEAr
  {L_S, L_E, L_L, L_F},  // SELF
  {L_L, L_E, L_E, L_t},  // LEEt
  {DIGITS[1], DIGITS[3], DIGITS[3], DIGITS[7]},  // 1337
  {L_S, L_H, L_I, L_I},  // SHII
  {DIGITS[6], DIGITS[7], DIGITS[6], DIGITS[7]},  // 6767
  {DIGITS[4], DIGITS[8], DIGITS[4], DIGITS[8]},  // 4848
  // ── Phase 5 additions ──
  {L_F, L_A, L_I, L_L},  // FAIL
  {L_b, L_o, L_o, L_t},  // boot
  {L_r, L_o, L_o, L_t},  // root
  {L_L, L_o, L_o, L_P},  // LOOP
  {L_o, L_o, L_P, L_S},  // ooPS
  {L_L, L_I, L_F, L_E},  // LIFE
  {L_S, L_U, L_d, L_o},  // SUdo
  {L_H, L_A, L_L, L_t},  // HALt
  {L_L, L_o, L_S, L_t},  // LoSt
  {L_L, L_o, L_A, L_d},  // LoAd
  {L_F, L_o, L_o, L_L},  // FooL
  {L_d, L_E, L_F, L_y},  // dEFy
  {L_A, L_C, L_I, L_d},  // ACId
  {L_H, L_A, L_S, L_H},  // HASH
  // ── 420 words (indices 33+) ──
  {L_H, L_I, L_G, L_H},  // HIGH
  {L_H, L_A, L_S, L_E},  // HASE  (haze, best effort on 7-seg)
  {L_L, L_E, L_A, L_F},  // LEAF
  {L_b, L_U, L_d, L_S},  // bUdS
  {DIGITS[4], DIGITS[2], DIGITS[0], L_o},  // 420o (420!)
  {L_C, L_H, L_I, L_L},  // CHIL  (chill)
  {L_d, L_o, L_P, L_E},  // doPE
  {L_H, L_E, L_r, L_b},  // HErb
};
const uint8_t NUM_EGGS = sizeof(EGGS) / sizeof(EGGS[0]);

// 420-specific egg indices (into EGGS array)
const uint8_t EGGS_420[] = {33, 34, 35, 36, 37, 38, 39, 40};
const uint8_t NUM_EGGS_420 = sizeof(EGGS_420) / sizeof(EGGS_420[0]);

// ── Random helpers ─────────────────────────────────────────────
inline bool chance(uint8_t pct) { return (os_random() % 100) < pct; }
inline uint8_t rng(uint8_t max) { return max > 0 ? os_random() % max : 0; }

// ── Color helpers ──────────────────────────────────────────────
inline uint8_t clamp8(int v) {
  return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

inline esphome::Color dim(esphome::Color c, uint8_t brightness) {
  return esphome::Color(
    c.r * brightness / 255,
    c.g * brightness / 255,
    c.b * brightness / 255
  );
}

inline esphome::Color shift_color(esphome::Color c) {
  int s = (int)rng(60) - 30;
  int b = (int)rng(40);
  return esphome::Color(clamp8(c.r + s), clamp8(c.g - s), clamp8(c.b + b));
}

inline esphome::Color wrong_color(esphome::Color c) {
  switch (rng(4)) {
    case 0:  return esphome::Color(c.b, c.r, c.g);
    case 1:  return esphome::Color(255 - c.r, 255 - c.g, 255 - c.b);
    case 2:  return esphome::Color(255, 0, 0);
    default: return esphome::Color(0, 0, 255);
  }
}

// HSV hue (0-255) → RGB with full saturation and value
inline esphome::Color hue_to_rgb(uint8_t hue) {
  uint8_t region = hue / 43;
  uint8_t frac = (hue - region * 43) * 6;
  switch (region) {
    case 0:  return esphome::Color(255, frac, 0);
    case 1:  return esphome::Color(255 - frac, 255, 0);
    case 2:  return esphome::Color(0, 255, frac);
    case 3:  return esphome::Color(0, 255 - frac, 255);
    case 4:  return esphome::Color(frac, 0, 255);
    default: return esphome::Color(255, 0, 255 - frac);
  }
}

// Detect special digit patterns: sequence (ascending) or repdigit (all same)
inline bool is_special_time(const int v[4]) {
  // All same: 00:00, 11:11, 22:22
  if (v[0] == v[1] && v[1] == v[2] && v[2] == v[3]) return true;
  // Ascending sequence: 01:23, 12:34, 23:45
  if (v[1] == v[0]+1 && v[2] == v[1]+1 && v[3] == v[2]+1) return true;
  // Descending sequence: 54:32, 43:21, 32:10 (rare valid times)
  if (v[1] == v[0]-1 && v[2] == v[1]-1 && v[3] == v[2]-1) return true;
  // Repeating pair: 12:12, 20:20, 23:23 etc.
  if (v[0] == v[2] && v[1] == v[3]) return true;
  // Palindrome: 12:21, 13:31, 23:32 etc.
  if (v[0] == v[3] && v[1] == v[2]) return true;
  return false;
}

// ── Warm pastel palette for special-time rainbow (6 stripes) ──
// Bright, warm, visible even at low display brightness.
// shimmer() adds per-call random variation for an alive feel.
inline esphome::Color shimmer(uint8_t r, uint8_t g, uint8_t b) {
  int jr = (int)rng(30) - 15;
  int jg = (int)rng(30) - 15;
  int jb = (int)rng(30) - 15;
  return esphome::Color(clamp8(r + jr), clamp8(g + jg), clamp8(b + jb));
}

// 6-stripe palette: warm pink → peach → cream → mint → sky → orchid
inline esphome::Color pastel_stripe(uint8_t idx) {
  switch (idx % 6) {
    case 0:  return shimmer(255, 110, 120);  // warm pink
    case 1:  return shimmer(255, 170,  90);  // peach
    case 2:  return shimmer(255, 235, 110);  // warm cream
    case 3:  return shimmer(140, 255, 160);  // soft mint
    case 4:  return shimmer(140, 150, 255);  // periwinkle
    default: return shimmer(235, 120, 255);  // orchid
  }
}

// ── Rendering ──────────────────────────────────────────────────
inline void set_seg(esphome::light::AddressableLight &it,
                    int base, int seg, int lps, esphome::Color c) {
  for (int j = 0; j < lps; j++)
    it[base + seg * lps + j] = c;
}

// Set only one LED within a segment pair (half-segment addressing)
inline void set_half_seg(esphome::light::AddressableLight &it,
                         int base, int seg, int lps, int which,
                         esphome::Color c) {
  it[base + seg * lps + (which % lps)] = c;
}

inline void render_digit(esphome::light::AddressableLight &it,
                         int base, int val, int lps, esphome::Color c) {
  uint8_t mask = DIGITS[val % 10];
  for (int seg = 0; seg < 7; seg++)
    if (mask & (1 << SEG_MAP[seg]))
      set_seg(it, base, seg, lps, c);
}

// Render digit with extra bitmask (for decay - mask away segments)
inline void render_digit_masked(esphome::light::AddressableLight &it,
                                int base, int val, int lps,
                                esphome::Color c, uint8_t extra_mask) {
  uint8_t pattern = DIGITS[val % 10] & extra_mask;
  for (int seg = 0; seg < 7; seg++)
    if (pattern & (1 << SEG_MAP[seg]))
      set_seg(it, base, seg, lps, c);
}

inline void render_colon(esphome::light::AddressableLight &it,
                         int offset, int n, esphome::Color c) {
  for (int i = 0; i < n; i++)
    it[offset + i] = c;
}

// Render digit with warm pastel stripes per horizontal line
// Each call generates slightly different colors (shimmer)
// Mapping (top→bottom): a=pink, f/b top=peach, f/b bot=cream,
//   g=mint, e/c top=sky, e/c bot=orchid, d=orchid
inline void render_digit_pride(esphome::light::AddressableLight &it,
                               int base, int val, int lps, uint8_t extra_mask) {
  uint8_t pattern = DIGITS[val % 10] & extra_mask;
  for (int seg = 0; seg < 7; seg++) {
    if (!(pattern & (1 << SEG_MAP[seg]))) continue;
    switch (seg) {
      case 5:  // a (top bar) → warm pink
        set_seg(it, base, seg, lps, pastel_stripe(0));
        break;
      case 6:  // f (upper-left)
      case 4:  // b (upper-right) → peach / cream
        if (lps >= 2) {
          set_half_seg(it, base, seg, lps, 0, pastel_stripe(1));
          set_half_seg(it, base, seg, lps, 1, pastel_stripe(2));
        } else {
          set_seg(it, base, seg, lps, pastel_stripe(1));
        }
        break;
      case 0:  // g (middle bar) → mint
        set_seg(it, base, seg, lps, pastel_stripe(3));
        break;
      case 1:  // e (lower-left)
      case 3:  // c (lower-right) → sky / orchid
        if (lps >= 2) {
          set_half_seg(it, base, seg, lps, 0, pastel_stripe(4));
          set_half_seg(it, base, seg, lps, 1, pastel_stripe(5));
        } else {
          set_seg(it, base, seg, lps, pastel_stripe(4));
        }
        break;
      case 2:  // d (bottom bar) → orchid
        set_seg(it, base, seg, lps, pastel_stripe(5));
        break;
    }
  }
}

// ── Artefact geometry ────────────────────────────────────────
// Horizontal rows: segment physical positions per row (255 = unused slot)
// Row 0=top(a), 1=upper verticals(f,b), 2=middle(g), 3=lower verticals(e,c), 4=bottom(d)
const uint8_t HROWS[][2]  = {{5,255}, {6,4}, {0,255}, {1,3}, {2,255}};
const uint8_t HROW_LEN[]  = {1, 2, 1, 2, 1};

// ── Digit morph transitions ─────────────────────────────────
// Tracks per-digit segment changes and animates the transition
// over 2 frames: first dim departing segments, then dim arriving segments
struct MorphState {
  uint8_t prev[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t phase[4] = {};  // 0=idle, 2=dimming old, 1=brightening new

  // Call each frame with current digit values
  void update(const int v[4]) {
    for (int d = 0; d < 4; d++) {
      uint8_t cur = (uint8_t)(v[d] % 10);
      if (phase[d] > 0) {
        phase[d]--;
      } else if (prev[d] != 0xFF && prev[d] != cur) {
        phase[d] = 2;  // start transition
      }
      if (phase[d] == 0) prev[d] = cur;
    }
  }

  // Render morph overlay — call AFTER normal digit rendering
  void render(esphome::light::AddressableLight &it,
              int offsets[4], const int v[4], int lps, esphome::Color c) {
    for (int d = 0; d < 4; d++) {
      if (phase[d] == 0 || prev[d] == 0xFF) continue;
      uint8_t old_mask = DIGITS[prev[d]];
      uint8_t new_mask = DIGITS[v[d] % 10];
      uint8_t target;
      if (phase[d] == 2) {
        // Phase 2: show departing segments (old & ~new) dimmed
        target = old_mask & ~new_mask;
      } else {
        // Phase 1: show arriving segments (new & ~old) dimmed
        target = new_mask & ~old_mask;
      }
      esphome::Color dc = dim(c, 100);  // ~40% brightness
      for (int seg = 0; seg < 7; seg++)
        if (target & (1 << SEG_MAP[seg]))
          set_seg(it, offsets[d], seg, lps, dc);
    }
  }
};

// ── 420 "Haze" palette (5 stripes) ──────────────────────────
// Deep greens, purple haze, amber, white smoke — slow shimmer
inline esphome::Color haze_shimmer(uint8_t r, uint8_t g, uint8_t b) {
  int jr = (int)rng(20) - 10;  // gentler than rainbow shimmer
  int jg = (int)rng(20) - 10;
  int jb = (int)rng(20) - 10;
  return esphome::Color(clamp8(r + jr), clamp8(g + jg), clamp8(b + jb));
}

inline esphome::Color haze_stripe(uint8_t idx) {
  switch (idx % 5) {
    case 0:  return haze_shimmer(  0, 180,  30);  // deep forest green
    case 1:  return haze_shimmer(120, 255,  50);  // lime haze
    case 2:  return haze_shimmer(160,  50, 220);  // purple haze
    case 3:  return haze_shimmer(255, 180,   0);  // amber glow
    default: return haze_shimmer(200, 220, 200);  // white smoke
  }
}

// Render digit with haze palette + breathing brightness
// Mapping: d=forest, e/c=lime/purple, g=amber, f/b=lime/smoke, a=smoke
inline void render_digit_haze(esphome::light::AddressableLight &it,
                              int base, int val, int lps, uint8_t extra_mask,
                              uint8_t breath_br) {
  uint8_t pattern = DIGITS[val % 10] & extra_mask;
  for (int seg = 0; seg < 7; seg++) {
    if (!(pattern & (1 << SEG_MAP[seg]))) continue;
    esphome::Color c;
    switch (seg) {
      case 2:  // d (bottom) → deep forest
        c = haze_stripe(0); break;
      case 1:  // e (lower-left)
      case 3:  // c (lower-right) → lime / purple
        if (lps >= 2) {
          set_half_seg(it, base, seg, lps, 0, dim(haze_stripe(1), breath_br));
          set_half_seg(it, base, seg, lps, 1, dim(haze_stripe(2), breath_br));
          continue;
        }
        c = haze_stripe(1); break;
      case 0:  // g (middle) → amber glow
        c = haze_stripe(3); break;
      case 6:  // f (upper-left)
      case 4:  // b (upper-right) → lime / smoke
        if (lps >= 2) {
          set_half_seg(it, base, seg, lps, 0, dim(haze_stripe(1), breath_br));
          set_half_seg(it, base, seg, lps, 1, dim(haze_stripe(4), breath_br));
          continue;
        }
        c = haze_stripe(1); break;
      case 5:  // a (top) → white smoke
      default:
        c = haze_stripe(4); break;
    }
    set_seg(it, base, seg, lps, dim(c, breath_br));
  }
}

} // namespace gc
