# VctrGameSim

**Native Windows Game Presence & Quest Scheduler by VctrLabDev** 
A lightweight, native C++17 Windows utility for simulating game presence naturally at the operating system level. Instead of injecting JavaScript into the Discord client or modifying internal modules, **VctrGameSim** spawns a minimal, zero-overhead Windows executable on the exact directory path Discord's native process scanner expects—wrapped in humanized scheduling and randomized session pacing.
---
**Not released you need to wait for V1.0.0 to use it still uunder construction**
## Why Native C++ Over Console Scripts?

Most public quest scripts rely on DevTools console injection, rigid metronome timers, and abrupt state changes that are easy to flag through client telemetry. **VctrGameSim** solves this cleanly at the OS level:

- **Zero Client Injection & Zero Token Risk:** Never touches Discord's internal `webpack` modules, never requires enabling DevTools, and never asks for your Discord Auth Token. Discord's built-in game detection picks up the process naturally.
- **Natural Pacing & Long Cooldowns (10m–2h):** Queue multiple games at once—the scheduler shuffles the queue order and inserts randomized breaks (ranging from **10 minutes to 2 hours** by default) between sessions so activity spreads naturally over the day.
- **Post-Completion Lingering:** Instead of killing the process the exact millisecond a timer finishes, the simulator stays active for an additional randomized window (**35–110 seconds**) before exiting.
- **Zero-Overhead Dummy Stub:** Uses a compiled ~15 KB Win32 windowless stub (`0% CPU`, `<2 MB RAM`) rather than heavy background applications.
- **Clean System State & Crash-Safe Cleanup:** Automatically checks if the real game is already running on your PC before starting, and binds the dummy process to a **Win32 Job Object**—guaranteeing that closing the tool immediately terminates the stub and wipes all temporary files from `%TEMP%`.
- **Smart Daily Limits:** Avoid running excessive sessions back-to-back. Sticking to **2–3 quests per day** keeps your account activity natural and avoids unnecessary attention.

---

## Supported Quest Types

- **`PLAY_ON_DESKTOP`** — Fully supported via native Windows process detection (automatically fetches official executable paths from Discord's detectable applications API).

> **Why aren't `WATCH_VIDEO`, `STREAM_ON_DESKTOP`, or `PLAY_ACTIVITY` included?**  
> Video and activity tasks cannot be completed via OS process detection—they require sending authenticated HTTP requests using your personal Discord Auth Token or hooking client modules. To keep **VctrGameSim** 100% token-free, client-safe, and transparent, those modules are intentionally excluded.

---

## Repository Structure

```text
VctrGameSim/
├── include/
│   └── json.hpp          # Single-header nlohmann/json library
├── src/
│   ├── stub.cpp          # Minimal windowless dummy executable (~15 KB, 0% CPU)
│   └── main.cpp          # Main scheduler, Discord API fetcher & Win32 Job controller
├── build.bat             # One-click MinGW-w64 build script
└── README.md
```

---

## Building from Source

### Prerequisites
- Windows 10 / 11
- **MinGW-w64 (`g++` with C++17 support)** or MSVC
- [`nlohmann/json.hpp`](https://github.com/nlohmann/json/releases/latest/download/json.hpp) placed inside the `include/` folder

### Compile
Run `build.bat` or execute the following commands in your terminal:

```bat
g++ -std=c++17 -O2 -mwindows src/stub.cpp -o stub.exe
g++ -std=c++17 -O2 -Iinclude src/main.cpp -lwinhttp -o VctrGameSim.exe
```

*(Ensure `stub.exe` and `VctrGameSim.exe` remain in the same directory.)*

---

## Usage

1. Open the **Discord Desktop App** and accept (**Enroll**) the `PLAY_ON_DESKTOP` quests you want to complete.
2. Launch **`VctrGameSim.exe`**.
   - *On first launch, it automatically downloads and caches Discord's detectable games database (`detectable_cache.json`).*
3. Search for each game by name or Application ID, select the matching number from the list, and set the target duration in minutes (press `Enter` for default `15` min).
4. Once you have added your games to the queue, type **`start`** and press `Enter`.
5. Set your desired minimum and maximum break intervals (or press `Enter` to use the default **10–120 minutes**) and let it run quietly in the background.

---

## Important Notes

- **Queue Multiple Quests at Once:** Add 2–3 games before typing `start` so the randomizer can shuffle the execution order and space them out with randomized cooldowns.
- **Keep Client Open & Prevent Sleep Mode:** Keep the Discord desktop app open and ensure your PC does not enter **Sleep mode** while the scheduler is counting down breaks or active sessions.
- **Instant Stop & Cleanup:** Press `Ctrl + C` or close the console window at any time—the Win32 Job Object immediately terminates any active stub process and removes temporary directories from `%TEMP%`.
- **Windows Defender / Antivirus Notice:** Because the tool copies an executable into `%TEMP%` and launches it without a visible window, heuristic antivirus scanners may occasionally flag it as a false positive. Both `main.cpp` and `stub.cpp` are 100% open-source so you can inspect every line and compile the binaries yourself.

---

## Disclaimer

This project is provided for educational and research purposes under the **VctrLabDev** open-source initiative. Use responsibly and at your own discretion.
