# Discord Quest Automator

A refined, low-profile automation script for completing accepted Discord Quests. Unlike standard public snippets that rely on rigid timers and abrupt state changes, this version is built around natural pacing and realistic session behavior.

## Why Use This Over Standard Scripts?

Most publicly available quest scripts follow predictable, automated patterns that are easy to flag through basic telemetry checks. This version addresses those flaws at a behavioral level:

* **Natural Pacing & Long Cooldowns:** Standard scripts run through every accepted quest back-to-back without stopping. This script shuffles your quest queue and introduces randomized breaks (ranging from 10 minutes to 2 hours) between tasks so activity looks spread out over a normal day.
* **Realistic Timing & Jitter:** Instead of sending progress updates on exact, metronomic intervals (e.g., every `7.0s` or `20.0s`), all heartbeats and checks use randomized intervals.
* **True 1x Playback & Post-Completion Linger:** Many older snippets advance video progress faster than real-time (`>1.0x` speed) and kill simulated games the exact millisecond a quest hits 100%. This version tracks real elapsed time and keeps the simulated game or stream active for a brief, randomized cooldown period after completion before closing.
* **Clean Client State:** Existing running games are preserved rather than overwritten, process metadata follows realistic formatting, and execution is isolated so nothing is left exposed in the global window scope.
* **Resilient Execution:** Automatically re-checks quest availability and expiration after long breaks and recovers cleanly from temporary network hiccups.

## Supported Quest Types

* `WATCH_VIDEO` / `WATCH_VIDEO_ON_MOBILE`
* `PLAY_ON_DESKTOP`
* `STREAM_ON_DESKTOP`
* `PLAY_ACTIVITY`

## Usage

1. Open the **Discord Desktop App** and accept (**Enroll**) the quests you want to complete.
2. Press `Ctrl + Shift + I` (`Cmd + Option + I` on macOS) to open Developer Tools.
   * *Note: If DevTools is disabled on your client, add `"DANGEROUS_ENABLE_DEVTOOLS_ONLY_ENABLE_IF_YOU_KNOW_WHAT_YOURE_DOING": true` to `%appdata%\discord\settings.json` and restart Discord.*
3. Switch to the **Console** tab.
4. If pasting is blocked by the browser console, type `allow pasting` and press `Enter`.
5. Paste the script (`Ctrl + V`) into the console and press `Enter`.

## Important Notes

* **Keep Client Open:** Because the script spaces out quests with randomized breaks, keep the Discord desktop app open and ensure your PC does not enter **Sleep mode** while it is running.
* **Stream Quests:** For `STREAM_ON_DESKTOP` tasks, manually join a voice channel with at least one other user and stream any window while the script is active.
* **Stop Execution:** Press `Ctrl + R` inside Discord at any time to reload the client and immediately stop the script.