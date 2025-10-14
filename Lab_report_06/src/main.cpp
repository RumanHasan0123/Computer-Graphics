// NAME: Gazi Ruman Hasan
// ID: 043231000101065
#include "glad.h"
#include "glfw3.h"
#include <iostream>
#include <vector>
#include <cmath>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Vertex shader: passes coordinates through
const char *vertexShaderSource = "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "void main(){ gl_Position = vec4(aPos, 1.0); }\n";

// Fragment shader: always draws white color
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main(){ FragColor = vec4(1.0, 1.0, 1.0, 1.0); }\n";

// Bresenham function (normalized coordinates)
std::vector<float> Bresenham(float x0, float y0, float x1, float y1) {
    std::vector<float> pts;
    int scale = 1000; // Higher scale for more points (smoother line)

    int X0 = int(round(x0 * scale));
    int Y0 = int(round(y0 * scale));
    int X1 = int(round(x1 * scale));
    int Y1 = int(round(y1 * scale));
    int dx = abs(X1 - X0);
    int dy = abs(Y1 - Y0);

    // Step direction for X axis
    int sx;
    if (X0 < X1)
        sx = 1;
    else
        sx = -1;

    // Step direction for Y axis
    int sy;
    if (Y0 < Y1)
        sy = 1;
    else
        sy = -1;

    int err = dx - dy; // Error term
    int x = X0;        // Current X position
    int y = Y0;        // Current Y position

    while (true) {
        // Convert back to normalized OpenGL coordinates
        pts.push_back(x / float(scale));
        pts.push_back(y / float(scale));
        pts.push_back(0.0f);
        if (x == X1 && y == Y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx)  { err += dx; y += sy; }
    }
    return pts;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Only R closes window
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "White Bresenham Line", NULL, NULL);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed to initialize GLAD\n"; return -1; }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader); glAttachShader(shaderProgram, fragmentShader); glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader); glDeleteShader(fragmentShader);

    // Bresenham line: perfectly straight and smooth diagonal (bottom-left to top-right)
    std::vector<float> line = Bresenham(-0.8f, -0.8f, 0.8f, 0.8f);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, 0, line.size() / 3);
        glfwSwapBuffers(window); glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
