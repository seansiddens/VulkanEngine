#include "first_app.hpp"

namespace ve {

void FirstApp::run() {
    while (!veWindow.shouldClose()) {
        glfwPollEvents();
    }
}

}  // namespace ve