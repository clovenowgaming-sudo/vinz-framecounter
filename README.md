# Frame Window Counter

Auto frame window counter. Play a level once, open the pause menu, press **Count**.
The mod replays your clicks at high speed and, for each click, finds how many ticks
earlier and later it can be while the run still survives until the next click.

## How it works
1. Every press/release you make is recorded with its physics tick (last completed run, or the attempt you pause in).
2. Press **Count** in the pause menu (not in practice mode).
3. For each press the mod tests offsets 0, -1, -2 ... and +1, +2 ... (max 60 each side).
4. A popup shows the window per click in ticks and ms (assuming 240 TPS).

## Limits
- A click passes if you survive until the next press. Later consequences are not checked.
- If the baseline (offset 0) fails, the click is reported as "baseline failed".
- Level must be played from the start (no start pos / checkpoints).

## Credits
- Inspired by **Antic's Counter** by anticroom. No code or assets from it are used.
