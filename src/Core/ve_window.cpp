#include "ve_window.hpp"

#include <stdexcept>

#include "ve_input.hpp"

namespace ve {

VeWindow::VeWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initWindow();
}

VeWindow::~VeWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VeWindow::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VeWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}

// Callback has to be static and void so we need to retrieve the window instance from the
// user pointer.
void VeWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    VeWindow &veWindow = reinterpret_cast<VeInput *>(glfwGetWindowUserPointer(window))->getWindow();
    veWindow.framebufferResized = true;
    veWindow.width = width;
    veWindow.height = height;
}

}  // namespace ve