# Frame Window Counter

Auto frame window counter with a built-in macro recorder and `.gdr2` import/export.

## Use
1. Pause > press the small **FC** button (bottom right).
2. **Record** restarts the level and records your inputs. Finish the level (or pause and press **Stop**).
   Or pick a `.gdr2` file from the import list (Download folder or the mod's `macros` folder).
3. **Count** replays the macro at 8x and tests offsets 0, -1, -2 ... / +1, +2 ... (max 60 each side).
4. A popup lists the window per click in ticks and ms (assuming 240 TPS).
5. **Export** saves the current macro as `.gdr2` (macros folder + Download).

## Limits
- A click passes if you survive until the next press. Later consequences are not checked.
- If the baseline (offset 0) fails the click is reported as "baseline failed".
- Level must be played from the start (no practice mode, start pos or checkpoints).

## Credits
- Inspired by **Antic's Counter** by anticroom. No code or assets from it are used.
- GDR2 format by **maxnut** (GDReplayFormat).
