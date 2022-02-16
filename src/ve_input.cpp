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

// Setters.
void VeInput::setKey(int key, int action) { keyState[key] = action; }

void VeInput::setMouseButton(int button, int action) { mouseState[button] = action; }

// TODO: IDK if this really does anything because the poll events function will overwrite position
//       based on the cursor's position in the window.
void VeInput::setMousePos(double xPos, double yPos) {
    // Save the previous mouse position.
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // Set current mouse position.
    mouseX = xPos;
    mouseY = yPos;
}

void VeInput::setInputMode(int mode, int value) {
    glfwSetInputMode(veWindow.getGLFWWindow(), mode, value);
}

// Getters.
bool VeInput::getKey(int key) {
    if (keyState.count(key) > 0) {
        return (keyState[key] == GLFW_PRESS || keyState[key] == GLFW_REPEAT);
    }

    return false;
}

bool VeInput::getMouseButton(int button) {
    if (mouseState.count(button) > 0) {
        return mouseState[button] == GLFW_PRESS;
    }

    return false;
}

double VeInput::getMouseX() const { return mouseX; }

double VeInput::getMouseY() const { return mouseY; }

double VeInput::getDeltaX() const { return deltaX; }

double VeInput::getDeltaY() const { return deltaY; }

// Input callback functions.
void VeInput::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->setKey(key, action);
}

void VeInput::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    auto *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->setMouseButton(button, action);
}
};  // namespace ve
