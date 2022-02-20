#include "ve_input.hpp"

#include <iostream>

namespace ve {

VeInput::VeInput(VeWindow &window) : veWindow{window} {
    // Set user pointer to point to this instance.
    glfwSetWindowUserPointer(veWindow.getGLFWWindow(), this);

    // Set callbacks.
    glfwSetKeyCallback(veWindow.getGLFWWindow(), keyCallback);
    glfwSetMouseButtonCallback(veWindow.getGLFWWindow(), mouseButtonCallback);

    // Initialize member variables.
    glfwGetCursorPos(veWindow.getGLFWWindow(), &mouseX, &mouseY);
    lastMouseX = mouseX;
    lastMouseY = mouseY;
}

void VeInput::pollEvents() {
    glfwPollEvents();

    // Save the previous mouse position.
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // Get current cursor position.
    glfwGetCursorPos(veWindow.getGLFWWindow(), &mouseX, &mouseY);

    // Compute the change in X and Y.
    deltaX = mouseX - lastMouseX;
    deltaY = mouseY - lastMouseY;
}

void VeInput::setInputMode(int mode, int value) {
    glfwSetInputMode(veWindow.getGLFWWindow(), mode, value);
}

// Getters -----------------------------------------------------------------------------------------
bool VeInput::getKey(int key) {
    return keyState[key];
}

bool VeInput::getMouseButton(int button) {
    return mouseState[button];
}

// Callbacks ---------------------------------------------------------------------------------------
// Input callback functions.
void VeInput::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->keyState[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
}

void VeInput::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    auto *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->mouseState[button] = action == GLFW_PRESS;
}
};  // namespace ve
