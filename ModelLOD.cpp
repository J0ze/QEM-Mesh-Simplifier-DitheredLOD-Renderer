#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "shader.h"
#include "fileSystem.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#include "LODmodel.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void DrawCall(Shader& shader, int LODlevel, LODModel& model);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

const unsigned int SCR_WIDTH = 1980;
const unsigned int SCR_HEIGHT = 1080;

Camera mainCamera(glm::vec3(0.0f, 0.0f, 10.0f));
bool firstMouse = true;
float lastX = 400, lastY = 300;

int main() {
#pragma region 预处理
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 导入函数
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 采用y轴反转
    stbi_set_flip_vertically_on_load(false);

    glEnable(GL_DEPTH_TEST);

    // 应用着色器
    Shader ourShader("model_vertex1.vs", "model_fragment1.fs");

    // 加载模型
    LODModel ourModel("C:/Users/34315/Downloads/test/bule.pmx");
#pragma endregion
    int currentLOD = 0;
    int nextLOD = 0;
    float currentFade = 1.0f;
    bool isTransitioning = false;

    // 定义切换阈值
    float LODThresholds[5] = { 15.0f, 30.0f, 50.0f, 80.0f, 120.0f };
    float transitionZone = 4.0f;
#pragma region 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float camDistance = abs(mainCamera.Position.z);;

        for (int i = 0; i < 5; i++) {
            float startZone = LODThresholds[i] - transitionZone / 2.0f;
            float endZone = LODThresholds[i] + transitionZone / 2.0f;

            if (camDistance < startZone) {
                currentLOD = i;
                break;
            }
            else if (camDistance >= startZone && camDistance <= endZone) {
                currentLOD = i;
                nextLOD = i + 1;
                isTransitioning = true;

                float distInZone = camDistance - startZone;
                currentFade = 1.0f - (distInZone / transitionZone);
                break;
            }

            if (i == 4 && camDistance > endZone) {
                // 超出最远距离，强制使用最低精度模型
                currentLOD = 5;
            }
        }

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // 视图 投影变换
        glm::mat4 projection = glm::perspective(glm::radians(mainCamera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = mainCamera.GetViewMatrix();
        ourShader.setMatrix4f("projection", projection);
        ourShader.setMatrix4f("view", view);

        // 模型变换
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); 
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setMatrix4f("model", model);

        if (isTransitioning) {
            ourShader.setFloat("u_fade", currentFade);
            ourShader.setBool("u_isInverted", false);
            DrawCall(ourShader, currentLOD, ourModel);

            ourShader.setFloat("u_fade", currentFade); 
            ourShader.setBool("u_isInverted", true);
            DrawCall(ourShader, nextLOD, ourModel);
        }
        else {
            ourShader.setFloat("u_fade", 1.0f);
            ourShader.setBool("u_isInverted", false);
            DrawCall(ourShader, currentLOD, ourModel);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
#pragma endregion
}

void DrawCall(Shader& shader, int LODlevel, LODModel &model) {
    switch (LODlevel) {
    case 0:
        model.Draw(shader, 1.0f);
        break;
    case 1:
        model.Draw(shader, 0.9f);
        break;
    case 2:
        model.Draw(shader, 0.8f);
        break;
    case 3:
        model.Draw(shader, 0.7f);
        break;
    case 4:
        model.Draw(shader, 0.6f);
        break;
    case 5:
        model.Draw(shader, 0.5f);
        break;     
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        mainCamera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mainCamera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mainCamera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mainCamera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    mainCamera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    mainCamera.ProcessMouseScroll(yoffset);
}
