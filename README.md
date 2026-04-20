# 🎮 Game Engine

A lightweight **2D game engine** built from scratch in C++ with Lua scripting. Designed around an entity-component architecture, it provides a flexible foundation for building 2D games with physics, rendering, audio, and input — all driven by simple Lua scripts.

---

## ✨ Features

- **Component-Based Architecture** — Actors are composed of reusable Lua components with lifecycle hooks (`OnStart`, `OnUpdate`, `OnLateUpdate`, `OnDestroy`)
- **Lua Scripting** — All game logic lives in Lua scripts, making iteration fast and accessible
- **Box2D Physics** — Full 2D physics with rigidbodies, colliders, triggers, and raycasting
- **Sprite & Text Rendering** — SDL2-powered renderer with camera system, zoom, sorting layers, and TTF font support
- **Audio System** — 50-channel audio mixer supporting `.wav` playback with looping and volume control
- **Particle System** — Configurable emitter with burst rates, velocity, color, gravity, drag, and pooling
- **Event Bus** — Publish/subscribe system for decoupled inter-component communication
- **Scene Management** — JSON-defined scenes with hot-swapping and persistent actors across transitions
- **Actor Templates** — Define reusable actor archetypes in JSON for easy spawning

---

## 🎮 Custom Feature: Controller Support

Full **gamepad/controller support** via SDL's `SDL_GameController` API, exposed to Lua as a clean, easy-to-use interface:

```lua
-- Check buttons
Input.GetButton("a")              -- held
Input.GetButtonDown("start")      -- just pressed
Input.GetButtonUp("right_bumper") -- just released

-- Read analog sticks & triggers (-1.0 to 1.0)
Input.GetAxis("left_x")
Input.GetAxis("right_trigger")

-- Connection status
Input.IsControllerConnected()
```

**Supported inputs:** A/B/X/Y, bumpers, triggers, D-pad, start/back, and both analog sticks. Includes **hot-plug detection** for connect/disconnect events and built-in **dead zone handling**. Multi-controller support is available via an optional controller ID parameter.

A demo component (`ControllerControl.lua`) showcases left stick + D-pad movement with automatic keyboard fallback.

---

## 🛠 Tech Stack

| Technology | Role |
|---|---|
| **C++17** | Core engine |
| **Lua** | Scripting language for all game logic |
| **LuaBridge** | C++ ↔ Lua bindings |
| **SDL2** | Windowing, rendering, input, and gamepad support |
| **SDL2_image** | Image/sprite loading |
| **SDL2_mixer** | Audio playback |
| **SDL2_ttf** | TrueType font rendering |
| **Box2D** | 2D physics simulation |
| **GLM** | Vector math |
| **RapidJSON** | Config and scene file parsing |

---

## 🚀 Building & Running

```bash
make main    # Build the engine
make run     # Build and run
make clean   # Clean build artifacts
```

---

## 📁 Project Structure

```
resources/
├── game.config              # Game title, initial scene
├── rendering.config         # Resolution, clear color
├── actor_templates/         # Reusable actor definitions (JSON)
├── component_types/         # Lua components (game logic lives here)
├── scenes/                  # Scene definitions (JSON)
├── audio/                   # Sound files
├── fonts/                   # TTF fonts
└── images/                  # Sprites and textures

src/
├── firstparty/              # Engine source (headers)
└── thirdparty/              # SDL, Box2D, Lua, GLM, RapidJSON, LuaBridge
```

