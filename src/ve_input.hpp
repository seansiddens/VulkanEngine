#pragma once

#include <unordered_map>

#include "ve_window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ve {

class VeInput {
   public:
    VeInput(VeWindow &window);

    // This function should be called once every frame.
    void pollEvents();

    void setKey(int key, int action);
    void setMouseButton(int button, int action);
    void setMousePos(double xPos, double yPos);

    int getKey(int key);
    int getMouseButton(int button);
    double getMouseX();
    double getMouseY();
    double getDeltaX();
    double getDeltaY();

    VeWindow &getWindow() const { return veWindow; }

   private:
    // Maps storing key and mouse button states.
    std::unordered_map<int, int> keyState;
    std::unordered_map<int, int> mouseState;

    // Mouse positions are measured in screen coordinates relative to the top-left corner
    // of the window content area (+X left, +Y down).
    // Current X and Y position of the mouse.
    double mouseX, mouseY;

    // Cursor position of when it was last polled.
    double lastMouseX;
    double lastMouseY;

    // Change in X and Y between frames.
    double deltaX{0.0}, deltaY{0.0};

    // Input callbacks.
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    // Reference to the window object.
    VeWindow &veWindow;
};

}  // namespace ve
