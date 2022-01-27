#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

// TODO: Validation errors when resizing window.

namespace ve {

class VeWindow {
   public:
    VeWindow(int w, int h, std::string name);
    ~VeWindow();

    // Remove copy constructors.
    VeWindow(const VeWindow &) = delete;
    VeWindow &operator=(const VeWindow &) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }
    [[nodiscard]] VkExtent2D getExtent() const { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
    [[nodiscard]] bool wasWindowResized() const { return framebufferResized; }
    void resetWindowResizedFlag() { framebufferResized = false; }
    [[nodiscard]] GLFWwindow *getGLFWWindow() const { return window; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

   private:
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    void initWindow();

    int width;
    int height;
    bool framebufferResized = false;

    std::string windowName;
    GLFWwindow *window;
};

}  // namespace ve