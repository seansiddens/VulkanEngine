#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace ve {

class VeWindow {
   public:
    VeWindow(int w, int h, std::string name);
    ~VeWindow();

    // Remove copy constructors.
    VeWindow(const VeWindow &) = delete;
    VeWindow &operator=(const VeWindow &) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

   private:
    void initWindow();

    const int width;
    const int height;

    std::string windowName;
    GLFWwindow *window;
};

}  // namespace ve