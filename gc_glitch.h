// GlitchClock - glitch engine & tunable parameters
#pragma once
#include "gc_core.h"

namespace gc {

// ── Tunable parameters (set from ESPHome globals each frame) ───
struct GlitchParams {
  float rate    = 0.5f;   // 0.0=clean .. 1.0=chaos
  float corrupt = 0.3f;   // 0.0=gentle .. 1.0=destructive
  float drift   = 0.5f;   // 0.0=stable .. 1.0=drunk clock
  uint8_t color = 0;      // 0=normal 1=shift
};

// ── Param-aware helpers ────────────────────────────────────────

// Scale a base percentage by glitch rate (rate=0.5 → unchanged)
inline uint8_t scaled_chance(uint8_t base_pct, float rate) {
  int v = (int)(base_pct * rate * 2.0f);
  return (uint8_t)(v > 100 ? 100 : (v < 0 ? 0 : v));
}

// Color transform for glitch-produced pixels
inline esphome::Color glitch_color(esphome::Color base, uint8_t mode) {
  if (mode == 1) return shift_color(base);
  return base;
}

// shift_color with corruption-scaled magnitude
inline esphome::Color shift_color_c(esphome::Color c, float corrupt) {
  int range = 30 + (int)(corrupt * 50.0f);
  int s = (int)(os_random() % (range * 2 + 1)) - range;
  int b = (int)(os_random() % (20 + (int)(corrupt * 40.0f) + 1));
  return esphome::Color(clamp8(c.r + s), clamp8(c.g - s), clamp8(c.b + b));
}

// ── Glitch state machine ──────────────────────────────────────
//
// Major effects: one at a time, rare, exclusive
// Minor effects: per-frame segment flicker, always possible
//
struct GlitchState {
  enum Major : uint8_t {
    NONE, TIME_WARP, DECAY, COLON_CHAOS, LOADING, SCRAMBLE, EASTER_EGG, ARTEFACT
  };

  GlitchParams p;

  Major effect = NONE;
  uint8_t ticks = 0;
  uint8_t variant = 0;      // sub-type selector
  int8_t  delta = 0;
  uint8_t decay_masks[4] = {0x7F, 0x7F, 0x7F, 0x7F};  // per-digit decay
  uint8_t decay_speed = 1;   // segments eroded per tick (corruption-scaled)
  uint8_t prev_hour = 255;
  bool afterhours = false;

  // Artefact: scanline sweep state
  uint8_t artefact_pos = 0;         // current scan position
  esphome::Color artefact_color;    // color picked at start

  // Shift mode: per-LED overlapping color waves with random palette
  static const int MAX_LEDS = 64;
  static const int PAL_SIZE = 8;
  uint8_t drift_steps[MAX_LEDS] = {};    // per-LED step counter
  esphome::Color drift_pal[PAL_SIZE];    // random color palette
  uint8_t drift_pal_head = 0;           // palette index for step 0
  uint8_t drift_pal_top = 0;            // highest generated step
  bool drift_active = false;

  // Generate a random color anchored to base with varying wildness
  esphome::Color drift_rand_color(esphome::Color base) {
    uint32_t r1 = os_random(), r2 = os_random();
    // 25% chance: adventurous (5-20% base), 75%: moderate (25-70% base)
    uint8_t keep = ((r2 >> 16) & 3) == 0
      ? 5 + (r1 % 16)
      : 25 + (r1 % 46);
    uint8_t rr = (r1 >> 8) & 0xFF;
    uint8_t rg = (r1 >> 16) & 0xFF;
    uint8_t rb = (r1 >> 24) & 0xFF;
    uint8_t br = 120 + (r2 % 136);  // brightness 120-255
    return esphome::Color(
      (uint8_t)((((int)base.r * keep + (int)rr * (100 - keep)) / 100) * br / 255),
      (uint8_t)((((int)base.g * keep + (int)rg * (100 - keep)) / 100) * br / 255),
      (uint8_t)((((int)base.b * keep + (int)rb * (100 - keep)) / 100) * br / 255)
    );
  }

  void set_params(const GlitchParams &params) { p = params; }

  bool is_420 = false;  // set each tick, used by render

  // ── Demo mode: ordered showcase of all effects ──
  // Phases: 0=clean, 1=rainbow, 2=haze, 3=time_warp, 4=decay,
  //         5=colon_chaos, 6=loading, 7=scramble, 8=easter_egg,
  //         9=artefact_h, 10=artefact_v, 11=color_drift
  static const uint8_t DEMO_PHASES = 12;
  bool demo = false;
  uint8_t demo_phase = 0;
  uint8_t demo_ticks = 0;

  // Returns: 0=normal, 1=rainbow, 2=haze (for palette override in lambda)
  uint8_t demo_palette() const {
    if (!demo) return 0;
    if (demo_phase == 1) return 1;  // rainbow
    if (demo_phase == 2) return 2;  // haze
    return 0;
  }

  static uint8_t demo_phase_duration(uint8_t phase) {
    //                  clean rainbow haze warp decay colon load scram egg  art_h art_v drift
    const uint8_t d[] = {  8,    8,   12,   8,  14,   10,   6,   4,  10,    8,    8,   12 };
    return phase < DEMO_PHASES ? d[phase] : 8;
  }

  void demo_enter_phase(uint8_t phase) {
    demo_phase = phase % DEMO_PHASES;
    demo_ticks = demo_phase_duration(demo_phase);

    // Reset state for clean entry
    effect = NONE;
    ticks = 0;
    for (int d = 0; d < 4; d++) decay_masks[d] = 0x7F;

    switch (demo_phase) {
      case 0:  // Clean - no effect
        break;
      case 1:  // Rainbow - handled by palette override in lambda
      case 2:  // Haze - handled by palette override in lambda
        break;
      case 3:  // Time warp
        effect = TIME_WARP;
        ticks = 99;  // held by demo timer
        variant = 0;
        delta = 3;
        break;
      case 4:  // Decay (spreading)
        effect = DECAY;
        ticks = 99;
        variant = 2;  // all-at-once
        decay_speed = 1;
        for (int d = 0; d < 4; d++)
          decay_masks[d] = 0x7F & ~(1 << rng(7));
        break;
      case 5:  // Colon chaos
        effect = COLON_CHAOS;
        ticks = 99;
        variant = 6;  // swing/wobble
        break;
      case 6:  // Loading
        effect = LOADING;
        ticks = 99;
        break;
      case 7:  // Scramble
        effect = SCRAMBLE;
        ticks = 99;
        break;
      case 8:  // Easter eggs (cycle through a few)
        effect = EASTER_EGG;
        ticks = 99;
        variant = 0;
        break;
      case 9:  // Artefact horizontal
        effect = ARTEFACT;
        ticks = 99;
        variant = 0;
        artefact_pos = 0;
        artefact_color = esphome::Color(255, 100, 0);
        break;
      case 10: // Artefact vertical
        effect = ARTEFACT;
        ticks = 99;
        variant = 2;
        artefact_pos = 0;
        artefact_color = esphome::Color(0, 150, 255);
        break;
      case 11: // Color drift - force shift mode temporarily
        break;
    }
  }

  // ── Tick: manage major effect lifecycle ──
  void tick(uint8_t hour = 255, uint8_t minute = 255) {
    is_420 = is_420_time(hour, minute);

    // ── Demo mode: sequenced showcase ──
    if (demo) {
      if (demo_ticks > 0) {
        demo_ticks--;
        // Animate decay during its phase
        if (effect == DECAY) {
          for (int d = 0; d < 4; d++) {
            if (decay_masks[d] == 0) continue;
            uint8_t bits[7]; int n = 0;
            for (int i = 0; i < 7; i++)
              if (decay_masks[d] & (1 << i)) bits[n++] = i;
            if (n > 0) decay_masks[d] &= ~(1 << bits[rng(n)]);
          }
        }
        // Cycle easter egg words every 3 ticks
        if (effect == EASTER_EGG && demo_ticks % 3 == 0)
          variant = (variant + 1) % NUM_EGGS;
        // Advance artefact scanline
        if (effect == ARTEFACT) artefact_pos++;
      } else {
        demo_enter_phase(demo_phase + 1);
      }
      return;
    }

    // ── Afterhours: at midnight, maybe forget to reset ──
    if (hour != prev_hour && hour != 255) {
      if (prev_hour == 255) {
        prev_hour = hour;
      } else {
        prev_hour = hour;
        if (hour == 0 && !afterhours) {
          afterhours = chance(scaled_chance(20, p.corrupt));
        } else if (afterhours && hour > 0 && hour <= 3) {
          if (chance(40)) afterhours = false;
        } else if (hour > 3) {
          afterhours = false;
        }
      }
    }

    if (ticks > 0) {
      ticks--;
      // Decay: erode segments across affected digits
      if (effect == DECAY) {
        for (int d = 0; d < 4; d++) {
          if (decay_masks[d] == 0x7F) continue;  // not yet hit
          for (int bite = 0; bite < decay_speed; bite++) {
            uint8_t bits[7]; int n = 0;
            for (int i = 0; i < 7; i++)
              if (decay_masks[d] & (1 << i)) bits[n++] = i;
            if (n > 0) decay_masks[d] &= ~(1 << bits[rng(n)]);
          }
        }
        // Spread: each tick, maybe start decaying another digit
        if (variant > 0 && chance(20 + (uint8_t)(p.corrupt * 30.0f))) {
          int d = rng(4);
          if (decay_masks[d] == 0x7F)
            decay_masks[d] = 0x7F & ~(1 << rng(7));  // knock out first segment
        }
      }
      if (effect == ARTEFACT) artefact_pos++;
      if (ticks == 0) effect = NONE;
      return;
    }

    // Roll for new major effect (rate scales the probability space)
    // rate=0.5 → space=200 (current), rate=1.0 → space=100, rate=0.0 → disabled
    if (p.rate < 0.01f) return;  // effectively off
    uint16_t space = (uint16_t)(200.0f / (p.rate * 2.0f));
    if (space < 14) space = 14;  // cap at ~7x normal frequency
    uint16_t roll = os_random() % space;

    if (roll == 0) {
      effect = LOADING;
      ticks = 3;
    } else if (roll == 1) {
      effect = SCRAMBLE;
      ticks = 1 + rng(2);
    } else if (roll < 5) {
      effect = TIME_WARP;
      ticks = 3 + rng(5);
      variant = rng(3);
      uint8_t max_delta = 2 + (uint8_t)(p.drift * 8.0f);
      delta = 1 + rng(max_delta);
    } else if (roll < 7) {
      effect = DECAY;
      // variant 0=single digit, 1=spreading, 2=all at once
      variant = rng(3);
      decay_speed = 1 + (uint8_t)(p.corrupt * 2.0f);
      ticks = (variant == 0) ? (4 + rng(5)) : (8 + rng(10));
      // Reset all masks to full
      for (int d = 0; d < 4; d++) decay_masks[d] = 0x7F;
      // Start decay on first digit(s)
      int start = rng(4);
      decay_masks[start] = 0x7F & ~(1 << rng(7));  // knock out first segment
      if (variant == 2) {
        // All-at-once: start decaying every digit immediately
        for (int d = 0; d < 4; d++)
          decay_masks[d] = 0x7F & ~(1 << rng(7));
      }
    } else if (roll < 9) {
      effect = ARTEFACT;
      // variant 0=horizontal down, 1=horizontal up, 2=vertical L→R
      variant = rng(3);
      artefact_pos = 0;
      artefact_color = (rng(2) == 0) ? shift_color(esphome::Color(255,255,255))
                                     : wrong_color(esphome::Color(255,255,255));
      ticks = 6 + rng(3);
    } else if (roll < 15) {
      effect = COLON_CHAOS;
      variant = rng(8);
      ticks = (variant >= 4) ? (8 + rng(16)) : (4 + rng(12));
    } else {
      // Easter egg: ultra rare, also scaled by rate
      // During 420: 10x more likely, picks from 420 subset
      uint32_t egg_space = (uint32_t)(4000.0f / (p.rate * 2.0f));
      if (egg_space < 500) egg_space = 500;
      if (is_420) egg_space /= 10;  // 10x boost during 420
      if ((os_random() % egg_space) == 0) {
        effect = EASTER_EGG;
        ticks = 3;
        if (is_420) {
          variant = EGGS_420[rng(NUM_EGGS_420)];
        } else {
          variant = rng(NUM_EGGS);
        }
      }
    }

    // Drift → probabilistic tick doubling (high drift accelerates cycling)
    static bool in_double = false;
    if (!in_double && p.drift > 0.5f && effect == NONE &&
        chance((uint8_t)((p.drift - 0.5f) * 40.0f))) {
      in_double = true;
      tick(hour);
      in_double = false;
    }
  }

  // ── Override: loading/scramble take over the whole display ──
  bool render_override(esphome::light::AddressableLight &it,
                       int offsets[4], int lps,
                       int colon_off, int colon_n,
                       esphome::Color c) {
    if (effect == LOADING) {
      int pos = (millis() / 80) % 6;
      for (int d = 0; d < 4; d++) {
        for (int t = 0; t < 3; t++) {
          int idx = (pos - t + 6) % 6;
          set_seg(it, offsets[d], SPIN[idx], lps, dim(c, 255 >> t));
        }
      }
      uint8_t phase = (millis() / 8) % 255;
      uint8_t pulse = phase < 128 ? phase * 2 : (255 - phase) * 2;
      render_colon(it, colon_off, colon_n, dim(c, pulse));
      return true;
    }
    if (effect == SCRAMBLE) {
      esphome::Color sc = glitch_color(c, p.color);
      uint8_t br = 180 + rng(76);
      for (int d = 0; d < 4; d++)
        render_digit(it, offsets[d], 8, lps, dim(sc, br));
      render_colon(it, colon_off, colon_n, dim(sc, br));
      return true;
    }
    if (effect == EASTER_EGG) {
      for (int d = 0; d < 4; d++)
        render_digit_masked(it, offsets[d], 8, lps, c, EGGS[variant][d]);
      render_colon(it, colon_off, colon_n, c);
      return true;
    }
    return false;
  }

  // ── Time warp / afterhours: show wrong time values ──
  void apply_time(int v[4]) {
    if (afterhours) {
      int h = v[0] * 10 + v[1] + 24;
      v[0] = h / 10;
      v[1] = h % 10;
    }
    if (effect != TIME_WARP) return;
    switch (variant) {
      case 0: {
        int fake = v[2] * 10 + v[3] + delta;
        v[2] = (fake / 10) % 10;
        v[3] = fake % 10;
        break;
      }
      case 1: {
        int fake = v[0] * 10 + v[1] + delta;
        v[0] = (fake / 10) % 10;
        v[1] = fake % 10;
        break;
      }
      case 2: {
        int d = rng(4);
        v[d] = (v[d] + delta) % 10;
        break;
      }
    }
  }

  // ── Decay: get segment mask for each digit ──
  void get_masks(uint8_t masks[4]) {
    if (effect == DECAY) {
      for (int d = 0; d < 4; d++) masks[d] = decay_masks[d];
    } else {
      masks[0] = masks[1] = masks[2] = masks[3] = 0x7F;
    }
  }

  // ── Minor: per-frame segment flicker + half-segment glitches ──
  void apply_segments(esphome::light::AddressableLight &it,
                      int offsets[4], int lps, esphome::Color c) {
    uint8_t flicker_pct = scaled_chance(3, p.rate);
    uint8_t half_pct = scaled_chance(2, p.rate);

    for (int d = 0; d < 4; d++) {
      // Full segment flicker
      if (chance(flicker_pct)) {
        int seg = rng(7);
        esphome::Color gc = glitch_color(c, p.color);
        uint8_t r = rng(4 + (int)(p.corrupt * 4.0f));
        if (r < 1)      set_seg(it, offsets[d], seg, lps, esphome::Color(0,0,0));
        else if (r < 3) set_seg(it, offsets[d], seg, lps, gc);
        else if (r < 5) set_seg(it, offsets[d], seg, lps, shift_color_c(c, p.corrupt));
        else             set_seg(it, offsets[d], seg, lps, wrong_color(c));
      }
      // Half-segment flicker
      if (lps >= 2 && chance(half_pct)) {
        int seg = rng(7);
        int which = rng(lps);
        esphome::Color gc = glitch_color(c, p.color);
        switch (rng(5)) {
          case 0: set_half_seg(it, offsets[d], seg, lps, which, esphome::Color(0,0,0)); break;
          case 1: set_half_seg(it, offsets[d], seg, lps, which, dim(gc, 80 + rng(100))); break;
          case 2: set_half_seg(it, offsets[d], seg, lps, which, shift_color_c(c, p.corrupt)); break;
          case 3: set_half_seg(it, offsets[d], seg, lps, which, wrong_color(c));               break;
          case 4: set_half_seg(it, offsets[d], seg, lps, which, dim(gc, rng(30)));             break;
        }
      }
    }
  }

  // ── Colon: normal blink, subtle imperfections, or chaos ──
  void apply_colon(esphome::light::AddressableLight &it,
                   int offset, int n, int second, esphome::Color c) {
    if (effect == COLON_CHAOS) {
      uint32_t ms = millis();
      switch (variant) {
        case 0: it[offset] = c; break;
        case 1: render_colon(it, offset, n, c); break;
        case 2: break;
        case 3:
          if (ms % 200 < 100) render_colon(it, offset, n, c);
          break;
        case 4:
          if (n >= 2) {
            if (ms % 1000 < 500) it[offset] = c;
            else                 it[offset + 1] = c;
          }
          break;
        case 5: {
          // Drift-scaled period: higher drift → more off-beat
          uint32_t period = 2000 + (uint32_t)(p.drift * 1000.0f);
          if (ms % period < period / 2) render_colon(it, offset, n, c);
          break;
        }
        case 6: {
          uint8_t w = (ms / 37) % 255;
          uint8_t swing = w < 128 ? w / 2 : (255 - w) / 2;
          if (second % 2 == 0 && n >= 2) {
            it[offset]     = dim(c, 255 - swing);
            it[offset + 1] = dim(c, 192 + swing);
          } else if (n >= 2) {
            it[offset + (swing > 32 ? 0 : 1)] = dim(c, 3 + rng(8));
          }
          break;
        }
        case 7:
          if (second % 2 == 0) {
            if (!chance(15)) render_colon(it, offset, n, c);
          } else {
            if (chance(8)) render_colon(it, offset, n, dim(c, 30 + rng(40)));
          }
          break;
      }
    } else {
      // Normal blink with very rare subtle imperfections (scaled by rate)
      if (second % 2 == 0) {
        if (n >= 2 && chance(scaled_chance(2, p.rate))) {
          it[offset] = c;
          it[offset + 1] = dim(c, 180 + rng(76));
        } else {
          render_colon(it, offset, n, c);
        }
      } else if (n >= 2 && chance(scaled_chance(1, p.rate))) {
        it[offset + rng(n)] = dim(c, rng(20));
      }
    }
  }

  // ── Artefact: scanline sweep across the display ──
  void apply_artefact(esphome::light::AddressableLight &it,
                      int offsets[4], int lps,
                      int colon_off, int colon_n, esphome::Color c) {
    if (effect != ARTEFACT) return;

    if (variant <= 1) {
      // Horizontal scan: sweep rows across all 4 digits
      for (int trail = 0; trail < 3; trail++) {
        int row = (int)artefact_pos - trail;
        if (variant == 1) row = 4 - ((int)artefact_pos - trail);  // bottom to top
        if (row < 0 || row > 4) continue;
        uint8_t br = 255 >> trail;  // 255, 127, 63
        esphome::Color ac = dim(artefact_color, br);
        for (int d = 0; d < 4; d++) {
          for (int s = 0; s < HROW_LEN[row]; s++) {
            uint8_t seg = HROWS[row][s];
            if (seg != 255) set_seg(it, offsets[d], seg, lps, ac);
          }
        }
        // Hit colon on middle rows
        if (row >= 1 && row <= 3)
          render_colon(it, colon_off, colon_n, ac);
      }
    } else {
      // Vertical scan: sweep digit columns left to right
      // Visual order: offsets[0]=digit1(left), [1]=digit2, colon, [2]=digit3, [3]=digit4(right)
      for (int trail = 0; trail < 2; trail++) {
        int col = (int)artefact_pos - trail;
        if (col < 0 || col > 4) continue;
        uint8_t br = 255 >> trail;
        esphome::Color ac = dim(artefact_color, br);
        if (col == 2) {
          // Colon column
          render_colon(it, colon_off, colon_n, ac);
        } else {
          // Map col to digit index: 0→0, 1→1, 3→2, 4→3
          int di = (col < 2) ? col : col - 1;
          render_digit(it, offsets[di], 8, lps, ac);  // all segments
        }
      }
    }
  }

  // ── Color drift: overlapping waves with random palette (Shift mode) ──
  //
  // Each LED has a step counter indexing into a rolling palette of random
  // colors. New palette entries are generated as the frontier advances.
  // Drift param controls overlap depth. Colors are random, anchored to
  // base with varying wildness and intensity.
  //
  void apply_color_drift(esphome::light::AddressableLight &it,
                         int num_leds, esphome::Color base) {
    if (p.color != 1) {
      drift_active = false;
      return;
    }

    int n = num_leds < MAX_LEDS ? num_leds : MAX_LEDS;

    if (!drift_active) {
      for (int i = 0; i < n; i++) drift_steps[i] = 0;
      drift_pal_head = 0;
      drift_pal_top = 0;
      drift_pal[0] = base;  // step 0 = current base color
      drift_active = true;
    }

    // Find current min/max steps
    uint8_t min_s = 255, max_s = 0;
    for (int i = 0; i < n; i++) {
      if (drift_steps[i] < min_s) min_s = drift_steps[i];
      if (drift_steps[i] > max_s) max_s = drift_steps[i];
    }

    // Max overlap window: drift param controls depth (1 at drift=0, ~4 at drift=2)
    int max_ahead = 1 + (int)(p.drift * 1.5f);

    // Snapshot for infection
    uint8_t snap[MAX_LEDS];
    for (int i = 0; i < n; i++) snap[i] = drift_steps[i];

    // ── Phase 1: spontaneous frontier advancement ──
    if ((int)(max_s - min_s) < max_ahead) {
      uint8_t seed_pct = scaled_chance(1, p.rate);
      if (seed_pct < 1) seed_pct = 1;
      for (int i = 0; i < n; i++) {
        if (snap[i] != max_s) continue;
        if (chance(seed_pct)) {
          drift_steps[i]++;
          // Generate palette entry for new step if needed
          uint8_t new_step = drift_steps[i];
          if (new_step > drift_pal_top) {
            drift_pal_top = new_step;
            uint8_t idx = (drift_pal_head + new_step) % PAL_SIZE;
            drift_pal[idx] = drift_rand_color(base);
          }
        }
      }
    }

    // ── Phase 2: neighbor infection ──
    for (int i = 0; i < n; i++) {
      uint8_t best = snap[i];
      bool from_pair = false;

      int pair = (i % 2 == 0) ? i + 1 : i - 1;
      if (pair >= 0 && pair < n && snap[pair] > best) {
        best = snap[pair];
        from_pair = true;
      }

      for (int d = -2; d <= 2; d += 4) {
        int adj = i + d;
        if (adj >= 0 && adj < n && snap[adj] > best)
          best = snap[adj];
      }

      if (best <= snap[i]) continue;

      uint8_t inf_pct = from_pair ? scaled_chance(8, p.rate)
                                  : scaled_chance(4, p.rate);
      if (chance(inf_pct))
        drift_steps[i] = snap[i] + 1;
    }

    // ── Phase 3: paint lit LEDs from palette ──
    for (int i = 0; i < n; i++) {
      auto px = it[i].get();
      uint8_t br = px.r > px.g ? (px.r > px.b ? px.r : px.b)
                                : (px.g > px.b ? px.g : px.b);
      if (br < 3) continue;

      uint8_t pal_idx = (drift_pal_head + drift_steps[i]) % PAL_SIZE;
      it[i] = dim(drift_pal[pal_idx], br);
    }

    // ── Phase 4: normalize + roll palette ──
    min_s = 255;
    for (int i = 0; i < n; i++)
      if (drift_steps[i] < min_s) min_s = drift_steps[i];

    if (min_s > 0) {
      for (int i = 0; i < n; i++) drift_steps[i] -= min_s;
      drift_pal_head = (drift_pal_head + min_s) % PAL_SIZE;
      drift_pal_top -= min_s;
    }
  }
};

} // namespace gc
