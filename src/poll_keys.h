GLFWwindow *RecreateWindow(GLFWwindow *old_window, int x, int y, int width, int height, bool borderless=false, bool topmost=false)
{
    glfwWindowHint(GLFW_VISIBLE, 0);
    glfwWindowHint(GLFW_DECORATED, !borderless);
    glfwWindowHint(GLFW_FLOATING, topmost);
    GLFWwindow *window = glfwCreateWindow(width,height,"ShaderPort",NULL,old_window);
    glfwDestroyWindow(old_window);
    glfwSetWindowPos(window, x, y);
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);

    // todo: make a function that do all of these and replace with here and start of main
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeChanged);
    glfwSetWindowFocusCallback(window, WindowFocusChanged);
    glfwSetWindowIconifyCallback(window, WindowIconifyChanged);
    ImGui_ImplGlfw_Init(window, true);

    return window;
}

void ImGui_ImplGlfw_PollKeys(GLFWwindow *window)
{
    ImGuiIO &io = ImGui::GetIO();
    int IMGUI_KEY_LAST = sizeof(io.KeysDown) / sizeof(io.KeysDown[0]); // todo: this might mess up if KeysDown is changed to not be a 512 array but a pointer
    for (int key = 0; key < GLFW_KEY_LAST && key < IMGUI_KEY_LAST; key++)
    {
        io.KeysDown[key] = glfwGetKey(window, key) == GLFW_PRESS;
    }
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}
