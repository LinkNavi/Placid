#ifndef KEYBIND_MANAGER_H
#define KEYBIND_MANAGER_H

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <string>
#include <functional>

class KeybindManager {
public:
    enum Action {
        MOVE_FORWARD,
        MOVE_BACKWARD,
        MOVE_LEFT,
        MOVE_RIGHT,
        JUMP,
        CROUCH,
        SPRINT,
        FIRE,
        AIM,
        RELOAD,
        SWITCH_WEAPON,
        USE,
        SCOREBOARD,
        CHAT,
        PAUSE
    };

private:
    std::unordered_map<Action, int> bindings;
    std::unordered_map<int, bool> keyStates;
    std::unordered_map<int, bool> mouseStates;

public:
    KeybindManager() {
        // Default bindings
        bindings[MOVE_FORWARD] = GLFW_KEY_W;
        bindings[MOVE_BACKWARD] = GLFW_KEY_S;
        bindings[MOVE_LEFT] = GLFW_KEY_A;
        bindings[MOVE_RIGHT] = GLFW_KEY_D;
        bindings[JUMP] = GLFW_KEY_SPACE;
        bindings[CROUCH] = GLFW_KEY_LEFT_CONTROL;
        bindings[SPRINT] = GLFW_KEY_LEFT_SHIFT;
        bindings[FIRE] = GLFW_MOUSE_BUTTON_LEFT;
        bindings[AIM] = GLFW_MOUSE_BUTTON_RIGHT;
        bindings[RELOAD] = GLFW_KEY_R;
        bindings[SWITCH_WEAPON] = GLFW_KEY_Q;
        bindings[USE] = GLFW_KEY_E;
        bindings[SCOREBOARD] = GLFW_KEY_TAB;
        bindings[CHAT] = GLFW_KEY_T;
        bindings[PAUSE] = GLFW_KEY_ESCAPE;
    }

    void SetBinding(Action action, int key) {
        bindings[action] = key;
    }

    int GetBinding(Action action) const {
        auto it = bindings.find(action);
        return (it != bindings.end()) ? it->second : -1;
    }

    void OnKeyEvent(int key, int action) {
        if (action == GLFW_PRESS) {
            keyStates[key] = true;
        } else if (action == GLFW_RELEASE) {
            keyStates[key] = false;
        }
    }

    void OnMouseButtonEvent(int button, int action) {
        if (action == GLFW_PRESS) {
            mouseStates[button] = true;
        } else if (action == GLFW_RELEASE) {
            mouseStates[button] = false;
        }
    }

    bool IsPressed(Action action) const {
        int key = GetBinding(action);
        if (key == -1) return false;
        
        // Check if it's a mouse button
        if (key >= GLFW_MOUSE_BUTTON_1 && key <= GLFW_MOUSE_BUTTON_LAST) {
            auto it = mouseStates.find(key);
            return (it != mouseStates.end()) ? it->second : false;
        }
        
        // Check if it's a keyboard key
        auto it = keyStates.find(key);
        return (it != keyStates.end()) ? it->second : false;
    }

    float GetAxis(Action positive, Action negative) const {
        float value = 0.0f;
        if (IsPressed(positive)) value += 1.0f;
        if (IsPressed(negative)) value -= 1.0f;
        return value;
    }

    // Serialize/deserialize for config files
    std::string Serialize() const {
        std::string result;
        for (const auto& [action, key] : bindings) {
            result += std::to_string(static_cast<int>(action)) + ":" + std::to_string(key) + ";";
        }
        return result;
    }

    void Deserialize(const std::string& data) {
        size_t pos = 0;
        while (pos < data.length()) {
            size_t colon = data.find(':', pos);
            size_t semi = data.find(';', colon);
            if (colon == std::string::npos || semi == std::string::npos) break;

            int action = std::stoi(data.substr(pos, colon - pos));
            int key = std::stoi(data.substr(colon + 1, semi - colon - 1));
            bindings[static_cast<Action>(action)] = key;

            pos = semi + 1;
        }
    }

    // Helper for display names
    static const char* GetActionName(Action action) {
        switch (action) {
            case MOVE_FORWARD: return "Move Forward";
            case MOVE_BACKWARD: return "Move Backward";
            case MOVE_LEFT: return "Move Left";
            case MOVE_RIGHT: return "Move Right";
            case JUMP: return "Jump";
            case CROUCH: return "Crouch";
            case SPRINT: return "Sprint";
            case FIRE: return "Fire";
            case AIM: return "Aim";
            case RELOAD: return "Reload";
            case SWITCH_WEAPON: return "Switch Weapon";
            case USE: return "Use/Interact";
            case SCOREBOARD: return "Scoreboard";
            case CHAT: return "Chat";
            case PAUSE: return "Pause";
            default: return "Unknown";
        }
    }

    static const char* GetKeyName(int key) {
        if (key >= GLFW_MOUSE_BUTTON_1 && key <= GLFW_MOUSE_BUTTON_LAST) {
            switch (key) {
                case GLFW_MOUSE_BUTTON_LEFT: return "Mouse Left";
                case GLFW_MOUSE_BUTTON_RIGHT: return "Mouse Right";
                case GLFW_MOUSE_BUTTON_MIDDLE: return "Mouse Middle";
                default: return "Mouse Button";
            }
        }

        switch (key) {
            case GLFW_KEY_SPACE: return "Space";
            case GLFW_KEY_LEFT_SHIFT: return "Left Shift";
            case GLFW_KEY_LEFT_CONTROL: return "Left Ctrl";
            case GLFW_KEY_ESCAPE: return "Escape";
            case GLFW_KEY_TAB: return "Tab";
            default:
                if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
                    static char buf[2] = {0};
                    buf[0] = 'A' + (key - GLFW_KEY_A);
                    return buf;
                }
                return "Key";
        }
    }
};

#endif
