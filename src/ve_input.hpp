#pragma once

#include <unordered_map>

#include "ve_window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ve {

// TODO: Make this a singleton (whatever that entails)?
class VeInput {
   public:
    explicit VeInput(VeWindow &window);

    // This function should be called once every frame.
    void pollEvents();

    void setInputMode(int mode, int value);

    // Returns whether a given key or mouse button is down.
    bool getKey(int key);
    bool getMouseButton(int button);

    // Get mouse position and change in position.
    std::pair<float, float> getMousePos() const { return {(float)mouseX, (float)mouseY}; }
    float getMouseX() const { return getMousePos().first; }
    float getMouseY() const { return getMousePos().second; }
    std::pair<float, float> getMouseDelta() const { return {(float)deltaX, (float)deltaY}; }
    float getMouseDeltaX() const { return getMouseDelta().first; }
    float getMouseDeltaY() const { return getMouseDelta().second; }

    VeWindow &getWindow() const { return veWindow; }

   private:
    // Maps storing key and mouse button states.
    std::unordered_map<int, bool> keyState;
    std::unordered_map<int, bool> mouseState;

    // Mouse position and delta are measured in screen coordinates relative to the top-left corner
    // of the window content area (+X left, +Y down).
    // Current X and Y position of the mouse.
    double mouseX{}, mouseY{};

    // Cursor position of when it was last polled.
    double lastMouseX;
    double lastMouseY;

    // Change in mouse position between frames.
    double deltaX{0.0}, deltaY{0.0};

    // Input callbacks.
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    // Reference to the window object.
    VeWindow &veWindow;
};

}  // namespace ve
