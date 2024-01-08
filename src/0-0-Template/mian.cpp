#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <tools/stb_image.h>
#include <tools/camera.h>
#include <tools/my_shader.h>
#include <tools/my_window.h>


#include <cmath>
#include <iostream>


using namespace std;

static void glfw_error_callback(int error, const char* description);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int loadTexture(const char* path);


// 每帧的图像大小
const unsigned int WIDTH = 1600;
const unsigned int HEIGHT = 1200;

// 记录每帧的渲染时间
float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间


//// 默认鼠标位置
bool firstMouse = true;
bool hideMouse = true;
bool catchMouse = true;
float lastX = WIDTH /2, lastY = HEIGHT /2;

Camera camera;
Window myWindow;

// 光源设置
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

// 物体沿某一个轴旋转
float angle = 0;
int axis_choose = 0;
glm::mat4 rotate_object_along_axis(glm::mat4 rotate, float angle, int axis_choose);

int main()
{
    myWindow.window_Init(WIDTH, HEIGHT, "5-1-BlinnPhong");

    const char* glsl_version = "#version 130";

    // 设置回调函数
    glfwSetCursorPosCallback(myWindow.window, mouse_callback);
    glfwSetScrollCallback(myWindow.window, scroll_callback);


    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // TODO：着色器、编译、链接
    Shader floorShader("shader/5-1-vertex.glsl", "shader/5-1-frag.glsl");


    // TODO：设置顶点坐标
    float planeVertices[] = {
        // positions            // normals         // texcoords
         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };

    // 物体VAO
    unsigned int cubeVAO, VBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // TODO: load Texture
    unsigned int floorTexture = loadTexture("texture/wood.png");

    // 设置imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // 设置imgui字体大小
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("D:/Download/imgui-1.90/misc/fonts/ProggyClean.ttf", 20.0f);

    // 设置imgui外观
    ImGui::StyleColorsDark();

    // 设置渲染后台
    ImGui_ImplGlfw_InitForOpenGL(myWindow.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // 启动深度测试
    glEnable(GL_DEPTH_TEST);

    // 渲染循环
    while (!glfwWindowShouldClose(myWindow.window))
    {
        // poll IO events (keys pressed/released, mouse moved etc.)
        glfwPollEvents();
        // input
        myWindow.processInput();
        processInput(myWindow.window);

        // 背景颜色设置
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        // 清除颜色缓冲和深度缓冲
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // 观察矩阵
        glm::mat4 view(1.0f);
        view = camera.GetViewMatrix();
        // 投影矩阵
        glm::mat4 projection(1.0f);
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);


        // 物体模型矩阵
        glm::mat4 model(1.0f);
        model = rotate_object_along_axis(model, angle, axis_choose);

        // 物体渲染器设置
        floorShader.use();

        // 光照计算
        floorShader.setVec3("lightPos", lightPos);
        floorShader.setVec3("viewPos", camera.Position);

        // 物体的材质
        floorShader.use();
        floorShader.setInt("material.diffuse", 0);
        floorShader.setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
        floorShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        floorShader.setFloat("material.shininess", 32.0f);

        // 光源性质
        //static glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 ambient = lightColor * glm::vec3(0.2f);
        glm::vec3 diffuse = lightColor * glm::vec3(0.5f);
        glm::vec3 specular = lightColor * glm::vec3(1.0f);
        floorShader.setVec3("light.ambient", ambient);
        floorShader.setVec3("light.diffuse", diffuse);
        floorShader.setVec3("light.specular", specular);

        // 模型、视角、投影矩阵设置
        floorShader.setMat4("model", model);
        floorShader.setMat4("view", view);
        floorShader.setMat4("projection", projection);

        static int lightMethod = 0;
        floorShader.setBool("LightMethod", lightMethod);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);


        // 启动imgui框架
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // 2. 显示自定义window，使用Begin/End pair创建命名window
        {
            ImGui::Begin("hello-GUI");                          // Create a window called "Hello, world!" and append into it.
            ImGui::Text("Texture mix value Setting");               // Display some text (you can use a format strings too)

            ImGui::Text("Lighting Method");
            ImGui::RadioButton("Phong Lighting", &lightMethod, 0); ImGui::SameLine();
            ImGui::RadioButton("Blinn-Phong Lighting", &lightMethod, 1); ImGui::SameLine();
            ImGui::NewLine();

            ImGui::Text("Rotate Axis Along");
            ImGui::RadioButton("Axis-x", &axis_choose, 0); ImGui::SameLine();
            ImGui::RadioButton("Axis-y", &axis_choose, 1); ImGui::SameLine();
            ImGui::RadioButton("Axis-z", &axis_choose, 2);
            ImGui::Text("Rotate angle");
            ImGui::SliderFloat("Angle", &angle, 0, 360);
            ImGui::NewLine();

            ImGui::Text("Light Color");
            ImGui::ColorEdit3("RGB", (float*)&lightColor); // Edit 3 floats representing a color
            ImGui::NewLine();

            ImGui::Text("BackGround Color Setting");
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
            ImGui::NewLine();

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 渲染每帧的时间
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // swap buffers
        glfwSwapBuffers(myWindow.window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    // TODO：缓冲对象、数组对象
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &VBO);


    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (hideMouse)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        hideMouse = false;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        catchMouse = false;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
    {
        hideMouse = true;
        catchMouse = true;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (catchMouse)
    {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.05;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

glm::mat4 rotate_object_along_axis(glm::mat4 rotate, float angle, int axis_choose)
{
    if (axis_choose == 0)
        rotate = glm::rotate(rotate, glm::radians(angle), glm::vec3(1.0, 0.0, 0.0));
    if (axis_choose == 1)
        rotate = glm::rotate(rotate, glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));
    if (axis_choose == 2)
        rotate = glm::rotate(rotate, glm::radians(angle), glm::vec3(0.0, 0.0, 1.0));

    return rotate;
}
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
