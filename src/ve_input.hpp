#pragma  once

#include "ve_window.hpp"

namespace ve {

class VeInput {
   public:
    VeInput();
    void pollEvents();

   private:
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);



};

} // namespace ve
