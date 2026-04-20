#ifndef CINPUT_H
#define CINPUT_H

#include "SDL/SDL.h"
#include "Input.h" // Reuse INPUT_STATE enum
#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Controller button name -> SDL_GameControllerButton mapping
const std::unordered_map<std::string, SDL_GameControllerButton> __button_name_to_sdl = {
    {"a", SDL_CONTROLLER_BUTTON_A},
    {"b", SDL_CONTROLLER_BUTTON_B},
    {"x", SDL_CONTROLLER_BUTTON_X},
    {"y", SDL_CONTROLLER_BUTTON_Y},
    {"back", SDL_CONTROLLER_BUTTON_BACK},
    {"guide", SDL_CONTROLLER_BUTTON_GUIDE},
    {"start", SDL_CONTROLLER_BUTTON_START},
    {"left_stick", SDL_CONTROLLER_BUTTON_LEFTSTICK},
    {"right_stick", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
    {"left_bumper", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
    {"right_bumper", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
    {"dpad_up", SDL_CONTROLLER_BUTTON_DPAD_UP},
    {"dpad_down", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
    {"dpad_left", SDL_CONTROLLER_BUTTON_DPAD_LEFT},
    {"dpad_right", SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
};

// Controller axis name -> SDL_GameControllerAxis mapping
const std::unordered_map<std::string, SDL_GameControllerAxis> __axis_name_to_sdl = {
    {"left_x", SDL_CONTROLLER_AXIS_LEFTX},
    {"left_y", SDL_CONTROLLER_AXIS_LEFTY},
    {"right_x", SDL_CONTROLLER_AXIS_RIGHTX},
    {"right_y", SDL_CONTROLLER_AXIS_RIGHTY},
    {"left_trigger", SDL_CONTROLLER_AXIS_TRIGGERLEFT},
    {"right_trigger", SDL_CONTROLLER_AXIS_TRIGGERRIGHT},
};

class CInput {
public:
    using ControllerID = SDL_JoystickID;

    // Per-controller state
    struct ControllerState {
        SDL_GameController* gc = nullptr;
        std::unordered_map<SDL_GameControllerButton, INPUT_STATE> button_states;
        std::vector<SDL_GameControllerButton> just_became_down_buttons;
        std::vector<SDL_GameControllerButton> just_became_up_buttons;
    };

    // Initialize controller subsystem. Call once at startup.
    static void Init() {
        if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
            SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
        }
        // Open all currently connected controllers
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                OpenController(i);
            }
        }
    }

    // Route SDL controller events. Called from the main event loop.
    static void ProcessEvent(const SDL_Event& e) {
        if (e.type == SDL_CONTROLLERDEVICEADDED) {
            // e.cdevice.which is the device index for ADDED events
            int device_index = e.cdevice.which;
            if (SDL_IsGameController(device_index)) {
                OpenController(device_index);
            }
        }
        else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            // e.cdevice.which is the instance ID for REMOVED events
            ControllerID id = e.cdevice.which;
            auto it = controllers.find(id);
            if (it != controllers.end()) {
                SDL_GameControllerClose(it->second.gc);
                controllers.erase(it);
                controller_order.erase(
                    std::remove(controller_order.begin(), controller_order.end(), id),
                    controller_order.end()
                );
            }
        }
        else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            ControllerID id = e.cbutton.which;
            auto it = controllers.find(id);
            if (it != controllers.end()) {
                auto btn = static_cast<SDL_GameControllerButton>(e.cbutton.button);
                it->second.button_states[btn] = INPUT_STATE_JUST_BECAME_DOWN;
                it->second.just_became_down_buttons.push_back(btn);
            }
        }
        else if (e.type == SDL_CONTROLLERBUTTONUP) {
            ControllerID id = e.cbutton.which;
            auto it = controllers.find(id);
            if (it != controllers.end()) {
                auto btn = static_cast<SDL_GameControllerButton>(e.cbutton.button);
                it->second.button_states[btn] = INPUT_STATE_JUST_BECAME_UP;
                it->second.just_became_up_buttons.push_back(btn);
            }
        }
    }

    // Transition one-frame states for all controllers.
    static void LateUpdate() {
        for (auto& [id, state] : controllers) {
            for (auto btn : state.just_became_down_buttons) {
                state.button_states[btn] = INPUT_STATE_DOWN;
            }
            state.just_became_down_buttons.clear();

            for (auto btn : state.just_became_up_buttons) {
                state.button_states[btn] = INPUT_STATE_UP;
            }
            state.just_became_up_buttons.clear();
        }
    }

    // --- Lua-exposed API (id = -1 means first connected controller) ---

    static bool GetButton(const std::string& button_name, int id = -1) {
        ControllerState* cs = ResolveController(id);
        if (!cs) return false;
        auto it = __button_name_to_sdl.find(button_name);
        if (it == __button_name_to_sdl.end()) return false;
        auto sit = cs->button_states.find(it->second);
        if (sit == cs->button_states.end()) return false;
        return sit->second == INPUT_STATE_DOWN || sit->second == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetButtonDown(const std::string& button_name, int id = -1) {
        ControllerState* cs = ResolveController(id);
        if (!cs) return false;
        auto it = __button_name_to_sdl.find(button_name);
        if (it == __button_name_to_sdl.end()) return false;
        auto sit = cs->button_states.find(it->second);
        if (sit == cs->button_states.end()) return false;
        return sit->second == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetButtonUp(const std::string& button_name, int id = -1) {
        ControllerState* cs = ResolveController(id);
        if (!cs) return false;
        auto it = __button_name_to_sdl.find(button_name);
        if (it == __button_name_to_sdl.end()) return false;
        auto sit = cs->button_states.find(it->second);
        if (sit == cs->button_states.end()) return false;
        return sit->second == INPUT_STATE_JUST_BECAME_UP;
    }

    static float GetAxis(const std::string& axis_name, int id = -1) {
        ControllerState* cs = ResolveController(id);
        if (!cs) return 0.0f;
        auto it = __axis_name_to_sdl.find(axis_name);
        if (it == __axis_name_to_sdl.end()) return 0.0f;
        float raw = SDL_GameControllerGetAxis(cs->gc, it->second) / 32767.0f;
        if (std::abs(raw) < deadzone) return 0.0f;
        return raw;
    }

    // id = -1 -> true if any controller connected; otherwise checks specific id
    static bool IsControllerConnected(int id = -1) {
        if (id == -1) return !controllers.empty();
        return controllers.find(static_cast<ControllerID>(id)) != controllers.end();
    }

    static void SetDeadzone(float value) {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        deadzone = value;
    }

    static float GetDeadzone() {
        return deadzone;
    }

    static int GetControllerCount() {
        return static_cast<int>(controllers.size());
    }

private:
    static void OpenController(int device_index) {
        SDL_GameController* gc = SDL_GameControllerOpen(device_index);
        if (gc) {
            ControllerID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
            if (controllers.find(id) == controllers.end()) {
                ControllerState state;
                state.gc = gc;
                controllers[id] = state;
                controller_order.push_back(id);
            }
        }
    }

    // id = -1 -> first connected controller; otherwise look up by id
    static ControllerState* ResolveController(int id) {
        if (id == -1) {
            if (controller_order.empty()) return nullptr;
            auto it = controllers.find(controller_order[0]);
            if (it == controllers.end()) return nullptr;
            return &it->second;
        }
        auto it = controllers.find(static_cast<ControllerID>(id));
        if (it == controllers.end()) return nullptr;
        return &it->second;
    }

    inline static std::unordered_map<ControllerID, ControllerState> controllers;
    inline static std::vector<ControllerID> controller_order; // insertion order
    inline static float deadzone = 0.1f;
};

#endif // CINPUT_H
