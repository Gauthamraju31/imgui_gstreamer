#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "pipeline.h"

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char **)
{

    //------------------------------------------------------------------
    // Boiler Plate stuff

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Gstreamer Pipeline", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Boiler Plate stuff
    //------------------------------------------------------------------

    // Our state
    bool play = false;
    bool pause = false;
    bool stop = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // gst_init (&argc, &argv);
    gst_init(NULL, NULL);
    Pipeline ImPipe;
    create_pipeline(&ImPipe);

    GLuint videotex;
    glGenTextures(1, &videotex);
    glBindTexture(GL_TEXTURE_2D, videotex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Grab a sample - https://github.com/tbeloqui/gst-imgui
        GstSample *videosample =
            gst_app_sink_try_pull_sample(GST_APP_SINK(ImPipe.appsink), 10 * GST_MSECOND);
        if (videosample)
        {
            GstBuffer *videobuf = gst_sample_get_buffer(videosample);
            GstMapInfo map;

            gst_buffer_map(videobuf, &map, GST_MAP_READ);

            glBindTexture(GL_TEXTURE_2D, videotex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, map.data);

            gst_buffer_unmap(videobuf, &map);
            gst_sample_unref(videosample);
        }

        ImGui::GetBackgroundDrawList()->AddImage( //
            (void *)(guintptr)videotex,           //
            ImVec2(0, 0),                         //
            ImVec2(1280, 720),                    //
            ImVec2(0, 0),                         //
            ImVec2(1, 1));

        ImGui::Begin("Gstreamer Pipeline Control");

        // Stream-Source
        ImGui::InputText("URI", ImPipe.stream_uri, IM_ARRAYSIZE(ImPipe.stream_uri));
        ImGui::Text("Control buttons for the pipeline");
        play = ImGui::Button("Play");
        ImGui::SameLine();
        pause = ImGui::Button("Pause");
        ImGui::SameLine();
        stop = ImGui::Button("Stop");
        ImGui::SameLine();
        ImGui::Checkbox("Loop",&ImPipe.loop);
        ImGui::End();

        if (play)
            ImPipe.pipeState = GST_STATE_PLAYING;
        else if (pause)
            ImPipe.pipeState = GST_STATE_PAUSED;
        else if (stop)
            ImPipe.pipeState = GST_STATE_NULL;

        update_pipeline_state(&ImPipe);
        check_pipeline_for_message(&ImPipe);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    cleanup_pipeline(&ImPipe);

    return 0;
}
