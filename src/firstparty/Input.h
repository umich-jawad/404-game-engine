#ifndef INPUT_H
#define INPUT_H

#include "SDL/SDL.h"
#include "glm/glm.hpp"
#include <unordered_map>
#include <vector>

const std::unordered_map<std::string, SDL_Scancode> __keycode_to_scancode = {
	// Directional (arrow) Keys
	{"up", SDL_SCANCODE_UP},
	{"down", SDL_SCANCODE_DOWN},
	{"right", SDL_SCANCODE_RIGHT},
	{"left", SDL_SCANCODE_LEFT},

	// Misc Keys
	{"escape", SDL_SCANCODE_ESCAPE},

	// Modifier Keys
	{"lshift", SDL_SCANCODE_LSHIFT},
	{"rshift", SDL_SCANCODE_RSHIFT},
	{"lctrl", SDL_SCANCODE_LCTRL},
	{"rctrl", SDL_SCANCODE_RCTRL},
	{"lalt", SDL_SCANCODE_LALT},
	{"ralt", SDL_SCANCODE_RALT},

	// Editing Keys
	{"tab", SDL_SCANCODE_TAB},
	{"return", SDL_SCANCODE_RETURN},
	{"enter", SDL_SCANCODE_RETURN},
	{"backspace", SDL_SCANCODE_BACKSPACE},
	{"delete", SDL_SCANCODE_DELETE},
	{"insert", SDL_SCANCODE_INSERT},

	// Character Keys
	{"space", SDL_SCANCODE_SPACE},
	{"a", SDL_SCANCODE_A},
	{"b", SDL_SCANCODE_B},
	{"c", SDL_SCANCODE_C},
	{"d", SDL_SCANCODE_D},
	{"e", SDL_SCANCODE_E},
	{"f", SDL_SCANCODE_F},
	{"g", SDL_SCANCODE_G},
	{"h", SDL_SCANCODE_H},
	{"i", SDL_SCANCODE_I},
	{"j", SDL_SCANCODE_J},
	{"k", SDL_SCANCODE_K},
	{"l", SDL_SCANCODE_L},
	{"m", SDL_SCANCODE_M},
	{"n", SDL_SCANCODE_N},
	{"o", SDL_SCANCODE_O},
	{"p", SDL_SCANCODE_P},
	{"q", SDL_SCANCODE_Q},
	{"r", SDL_SCANCODE_R},
	{"s", SDL_SCANCODE_S},
	{"t", SDL_SCANCODE_T},
	{"u", SDL_SCANCODE_U},
	{"v", SDL_SCANCODE_V},
	{"w", SDL_SCANCODE_W},
	{"x", SDL_SCANCODE_X},
	{"y", SDL_SCANCODE_Y},
	{"z", SDL_SCANCODE_Z},
	{"0", SDL_SCANCODE_0},
	{"1", SDL_SCANCODE_1},
	{"2", SDL_SCANCODE_2},
	{"3", SDL_SCANCODE_3},
	{"4", SDL_SCANCODE_4},
	{"5", SDL_SCANCODE_5},
	{"6", SDL_SCANCODE_6},
	{"7", SDL_SCANCODE_7},
	{"8", SDL_SCANCODE_8},
	{"9", SDL_SCANCODE_9},
	{"/", SDL_SCANCODE_SLASH},
	{";", SDL_SCANCODE_SEMICOLON},
	{"=", SDL_SCANCODE_EQUALS},
	{"-", SDL_SCANCODE_MINUS},
	{".", SDL_SCANCODE_PERIOD},
	{",", SDL_SCANCODE_COMMA},
	{"[", SDL_SCANCODE_LEFTBRACKET},
	{"]", SDL_SCANCODE_RIGHTBRACKET},
	{"\\", SDL_SCANCODE_BACKSLASH},
	{"'", SDL_SCANCODE_APOSTROPHE}
};
enum INPUT_STATE { INPUT_STATE_UP, INPUT_STATE_JUST_BECAME_DOWN, INPUT_STATE_DOWN, INPUT_STATE_JUST_BECAME_UP };

class Input
{
public:
	static void init() {
        for (int code = SDL_SCANCODE_UNKNOWN; code < SDL_NUM_SCANCODES; code++) {
            keyboard_states[static_cast<SDL_Scancode>(code)] = INPUT_STATE_UP;
        }
        for (int button = 1; button <= 3; button++) {
            mouse_button_states[button] = INPUT_STATE_UP;
        }
    }

	static void ProcessEvent(const SDL_Event & e) {
        if (e.type == SDL_KEYDOWN) {
            SDL_Scancode scancode = e.key.keysym.scancode;
            keyboard_states[scancode] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_scancodes.push_back(scancode);
        }
        else if (e.type == SDL_KEYUP) {
            SDL_Scancode scancode = e.key.keysym.scancode;
            keyboard_states[scancode] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_scancodes.push_back(scancode);
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            int button = e.button.button;
            mouse_button_states[button] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_buttons.push_back(button);
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            int button = e.button.button;
            mouse_button_states[button] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_buttons.push_back(button);
        }
        else if (e.type == SDL_MOUSEWHEEL) {
            mouse_scroll_this_frame = e.wheel.preciseY;
        }
        else if (e.type == SDL_MOUSEMOTION) {
            mouse_position = glm::ivec2(e.motion.x, e.motion.y);
        }
    }

	static void LateUpdate() {
        for (SDL_Scancode scancode : just_became_down_scancodes) {
            keyboard_states[scancode] = INPUT_STATE_DOWN;
        }
        just_became_down_scancodes.clear();

        for (SDL_Scancode scancode : just_became_up_scancodes) {
            keyboard_states[scancode] = INPUT_STATE_UP;
        }
        just_became_up_scancodes.clear();

        for (int button : just_became_down_buttons) {
            mouse_button_states[button] = INPUT_STATE_DOWN;
        }
        just_became_down_buttons.clear();

        for (int button : just_became_up_buttons) {
            mouse_button_states[button] = INPUT_STATE_UP;
        }
        just_became_up_buttons.clear();

        mouse_scroll_this_frame = 0.0f; // Reset scroll delta each frame
    }

	static bool GetKey(const std::string& keycode) {
        auto it = __keycode_to_scancode.find(keycode);
        if (it == __keycode_to_scancode.end()) return false;
        return keyboard_states[it->second] == INPUT_STATE_DOWN || keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_DOWN;
    }

	static bool GetKeyDown(const std::string& keycode) {
        auto it = __keycode_to_scancode.find(keycode);
        if (it == __keycode_to_scancode.end()) return false;
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_DOWN;
    }

	static bool GetKeyUp(const std::string& keycode) {
        auto it = __keycode_to_scancode.find(keycode);
        if (it == __keycode_to_scancode.end()) return false;
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_UP;
    }

    static glm::vec2 GetMousePosition() { // return as a vector for easier use with glm functions
        return mouse_position;
    }

	static bool GetMouseButton(int button) {
        return mouse_button_states[button] == INPUT_STATE_DOWN || mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
    }

	static bool GetMouseButtonDown(int button) {
        return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
    }

	static bool GetMouseButtonUp(int button) {
        return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP;
    }

    static float GetMouseScrollDelta() {
        return mouse_scroll_this_frame;
    }

	static void HideCursor() {
        SDL_ShowCursor(SDL_DISABLE);
    }

    static void ShowCursor() {
        SDL_ShowCursor(SDL_ENABLE);
    }

private:
	inline static std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
	inline static std::vector<SDL_Scancode> just_became_down_scancodes;
	inline static std::vector<SDL_Scancode> just_became_up_scancodes;

    inline static glm::ivec2 mouse_position;

	inline static std::unordered_map<int, INPUT_STATE> mouse_button_states;
	inline static std::vector<int> just_became_down_buttons;
	inline static std::vector<int> just_became_up_buttons;

    inline static float mouse_scroll_this_frame = 0.0f; // Positive for scroll up, negative for scroll down, reset to 0 each frame
};

#endif // INPUT_H