#pragma  once

#include "ve_window.hpp"

#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ve {

class VeInput {
   public:
    VeInput(VeWindow &window);

    void pollEvents();

    void setKey(int key, int action);
    void setMouseButton(int button, int action);

    int getKey(int key);
    int getMouseButton(int button);

    VeWindow &getWindow() const {return veWindow; }

    double cursorXPos;
    double cursorYPos;

   private:
    // Maps storing key and mouse button states.
    std::unordered_map<int, int> keyState;
    std::unordered_map<int, int> mouseState;
    
    // Input callbacks.
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    VeWindow &veWindow;



};

} // namespace ve
