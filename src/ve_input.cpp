//
// Created by sean on 1/23/22.
//

#include "ve_input.hpp"

#include <iostream>

namespace ve {

VeInput::VeInput(VeWindow &window) : veWindow{window} {  
    // Set user pointer to point to this instance.
    glfwSetWindowUserPointer(veWindow.getGLFWWindow(), this);
    glfwSetKeyCallback(veWindow.getGLFWWindow(), keyCallback);
}

void VeInput::pollEvents() {
    glfwPollEvents();

    glfwGetCursorPos(veWindow.getGLFWWindow(), &cursorXPos, &cursorYPos);
}

void VeInput::setKey(int key, int action) {
    keyState[key] = action;
}

void VeInput::setMouseButton(int button, int action) {
    mouseState[button] = action;
}

int VeInput::getKey(int key) {
    if (keyState.count(key) > 0) {
        return keyState[key];
    }

    return GLFW_KEY_UNKNOWN;
}

void VeInput::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    VeInput *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->setKey(key, action);
}

void VeInput::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    VeInput *input = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window));

    input->setMouseButton(button, action);
}

};

