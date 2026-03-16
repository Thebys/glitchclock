# GlitchClock - ESPHome external component
# Provides gc_core.h and gc_glitch.h for lambda effects.

import esphome.codegen as cg
import esphome.config_validation as cv
from pathlib import Path

CODEOWNERS = ["@Thebys"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    cg.add_define("USE_GLITCHCLOCK")
    # Add component directory to include path so lambdas can use
    # #include "gc_core.h" directly (without esphome/components/... prefix)
    comp_dir = Path(__file__).parent
    cg.add_build_flag(f"-I{comp_dir}")
