#include "glad.h"
#include "glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include "shader_m.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>

// ============================================================================
// SHADER SOURCE CODE
// ============================================================================

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 transform;\n"
"void main()\n"
"{\n"
"   gl_Position = transform * vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = ourColor;\n"
"}\n\0";

// ============================================================================
// SCREEN CONFIGURATION
// ============================================================================

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// ============================================================================
// GAME STATE VARIABLES
// ============================================================================

// Pillar opacity (alpha values)
float pillarOpacity[4] = {1.0f, 1.0f, 1.0f, 1.0f};
const float OPACITY_FADE_RATE = 0.2f;      // Opacity decreases 0.2 per second
const float OPACITY_GAIN_RATE = 0.25f;     // Opacity increases per space press
const float OPACITY_THRESHOLD = 0.55f;     // Minimum to survive
const float GAME_DURATION = 10.0f;         // 10 seconds to win

// Game state
float gameTimer = 0.0f;
bool gameActive = true;
bool gameWon = false;
bool gameLost = false;
std::string gameStatus = "Keep Clicking!";

// Boom animation variables
bool boomStarted = false;
float boomTimer = 0.0f;
const float BOOM_DURATION = 2.5f;
std::vector<bool> componentDisappeared;

// Background color feedback (changes as opacity decreases)
float bgRed = 1.0f, bgGreen = 1.0f, bgBlue = 1.0f;

// ============================================================================
// BUILDING COMPONENT STRUCTURE
// ============================================================================

struct BuildingComponent {
    float x, y;           // Position (center)
    float width, height;  // Dimensions
    glm::vec3 color;      // Color (R, G, B)
    unsigned int VAO, VBO;
    float disappearTime;  // When to start disappearing
    
    BuildingComponent(float x, float y, float w, float h, glm::vec3 col, float disTime)
        : x(x), y(y), width(w), height(h), color(col), disappearTime(disTime) {
        setupGeometry();
    }
    
    void setupGeometry() {
        float vertices[] = {
            0.5f,  0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f 
        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    void draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    ~BuildingComponent() {
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
};

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void createBuildingComponents(std::vector<BuildingComponent*>& components);
void updateBoomAnimation(float deltaTime);
void resetGame();
void updateGameLogic(float deltaTime);
void updateBackgroundColor();

// ============================================================================
// GLOBAL BUILDING COMPONENTS
// ============================================================================

std::vector<BuildingComponent*> buildingComponents;

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "UITS Pillar Builder - Clicker Game",
                                          NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    createBuildingComponents(buildingComponents);
    componentDisappeared.resize(buildingComponents.size(), false);

    glm::mat4 projection = glm::ortho(-6.0f, 6.0f, -5.0f, 5.0f, -1.0f, 1.0f);

    // Background starts WHITE
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    float lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window);
        updateGameLogic(deltaTime);
        updateBackgroundColor();

        if (boomStarted)
        {
            updateBoomAnimation(deltaTime);
        }

        // Update background color based on game state
        glClearColor(bgRed, bgGreen, bgBlue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Handle background flash effect during boom
        if (boomStarted)
        {
            float flashIntensity = std::sin(boomTimer * 6.28f * 5.0f) * 0.5f + 0.5f;
            glClearColor(
                0.5f + flashIntensity * 0.5f,
                0.2f + flashIntensity * 0.3f,
                0.2f + flashIntensity * 0.3f,
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // Draw each building component
        for (size_t i = 0; i < buildingComponents.size(); i++)
        {
            BuildingComponent* comp = buildingComponents[i];

            glm::mat4 model = glm::mat4(1.0f);

            // Check if component should disappear
            float timeSinceDisappear = boomTimer - comp->disappearTime;

            if (timeSinceDisappear > 0.0f && boomStarted)
            {
                float disappearProgress = timeSinceDisappear / 0.5f;
                disappearProgress = std::min(disappearProgress, 1.0f);

                if (disappearProgress >= 1.0f)
                {
                    componentDisappeared[i] = true;
                    continue;
                }

                model = glm::translate(model, glm::vec3(comp->x, comp->y, 0.0f));

                float shakeAmount = (1.0f - disappearProgress) * 0.1f;
                float shakeX = std::sin(boomTimer * 20.0f) * shakeAmount;
                float shakeY = std::cos(boomTimer * 25.0f) * shakeAmount;
                model = glm::translate(model, glm::vec3(shakeX, shakeY, 0.0f));

                float rotationAngle = disappearProgress * 3.14159f * 2.0f;
                model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));

                float scaleAmount = 1.0f - (disappearProgress * 0.8f);
                model = glm::scale(model, glm::vec3(scaleAmount, scaleAmount, 1.0f));

                model = glm::scale(model, glm::vec3(comp->width, comp->height, 1.0f));
            }
            else if (!componentDisappeared[i])
            {
                model = glm::translate(model, glm::vec3(comp->x, comp->y, 0.0f));
                model = glm::scale(model, glm::vec3(comp->width, comp->height, 1.0f));
            }
            else
            {
                continue;
            }

            glm::mat4 transform = projection * model;

            unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

            // Apply opacity to pillars
            glm::vec4 finalColor = glm::vec4(comp->color, 1.0f);
            
            // Check if this is a pillar (indices 5-8: pillars are at 5, 6, 7, 8)
            if (i >= 5 && i <= 8 && gameActive && !boomStarted)
            {
                int pillarIndex = i - 5;
                finalColor.w = pillarOpacity[pillarIndex];
            }

            int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
            glUniform4f(vertexColorLocation, finalColor.x, finalColor.y, finalColor.z, finalColor.w);

            comp->draw();
        }

        // Print game status to console with real-time feedback
        float avgOpacity = (pillarOpacity[0] + pillarOpacity[1] + pillarOpacity[2] + pillarOpacity[3]) / 4.0f;
        std::cout << "\rTimer: " << std::fixed << std::setprecision(1) 
                  << (GAME_DURATION - gameTimer) << "s | Avg Opacity: " << std::setprecision(2) << avgOpacity
                  << " | P1: " << pillarOpacity[0] 
                  << " | P2: " << pillarOpacity[1] 
                  << " | P3: " << pillarOpacity[2] 
                  << " | P4: " << pillarOpacity[3] 
                  << " | Status: " << gameStatus << "       " << std::flush;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (auto comp : buildingComponents)
    {
        delete comp;
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

// ============================================================================
// CALLBACK: Handle Window Resize
// ============================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // SPACE: Boost pillar opacity
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        if (!spacePressed && gameActive && !boomStarted)
        {
            for (int i = 0; i < 4; i++)
            {
                pillarOpacity[i] = std::min(pillarOpacity[i] + OPACITY_GAIN_RATE, 1.0f);
            }
            spacePressed = true;
        }
    }
    else
    {
        spacePressed = false;
    }

    // R key to reset
    static bool rPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        if (!rPressed)
        {
            resetGame();
            rPressed = true;
        }
    }
    else
    {
        rPressed = false;
    }
}

// ============================================================================
// UPDATE BACKGROUND COLOR BASED ON OPACITY
// ============================================================================

void updateBackgroundColor()
{
    // Calculate average opacity
    float avgOpacity = (pillarOpacity[0] + pillarOpacity[1] + pillarOpacity[2] + pillarOpacity[3]) / 4.0f;

    // Background transitions from WHITE (1,1,1) to RED (1,0,0) as opacity decreases
    if (!gameLost && gameActive)
    {
        // Interpolate: when avgOpacity is 1.0, bg is white; when avgOpacity is 0.55, bg turns more red
        float opacityRatio = (avgOpacity - OPACITY_THRESHOLD) / (1.0f - OPACITY_THRESHOLD);
        opacityRatio = std::max(opacityRatio, 0.0f); // Clamp to 0 minimum

        bgRed = 1.0f;
        bgGreen = opacityRatio;  // Transitions from 1.0 (white) to 0.0 (red)
        bgBlue = opacityRatio;   // Transitions from 1.0 (white) to 0.0 (red)
    }
    else if (gameLost)
    {
        // During loss, keep red background
        bgRed = 1.0f;
        bgGreen = 0.2f;
        bgBlue = 0.2f;
    }
    else if (gameWon)
    {
        // On win, turn green
        bgRed = 0.2f;
        bgGreen = 1.0f;
        bgBlue = 0.2f;
    }
}

// ============================================================================
// GAME LOGIC UPDATE
// ============================================================================

void updateGameLogic(float deltaTime)
{
    if (!gameActive || boomStarted)
        return;

    // Decrease pillar opacity over time (fade rate: 0.2 per second)
    for (int i = 0; i < 4; i++)
    {
        pillarOpacity[i] -= OPACITY_FADE_RATE * deltaTime;
        pillarOpacity[i] = std::max(pillarOpacity[i], 0.0f);
    }

    // Check if any pillar is below threshold
    bool anyPillarFailed = false;
    for (int i = 0; i < 4; i++)
    {
        if (pillarOpacity[i] < OPACITY_THRESHOLD)
        {
            anyPillarFailed = true;
            break;
        }
    }

    if (anyPillarFailed)
    {
        // LOSE: Trigger boom
        gameActive = false;
        gameLost = true;
        gameStatus = "LOST! Pillars collapsed!";
        boomStarted = true;
        boomTimer = 0.0f;
        std::cout << "\n>>> GAME LOST! Pillars collapsed! <<<\n";
        return;
    }

    // Increase game timer
    gameTimer += deltaTime;

    if (gameTimer >= GAME_DURATION)
    {
        // WIN: Time passed with all pillars above threshold
        gameActive = false;
        gameWon = true;
        gameStatus = "WON! Campus Saved!";
        std::cout << "\n>>> GAME WON! Campus is safe! <<<\n";
    }
}

// ============================================================================
// BOOM ANIMATION UPDATE
// ============================================================================

void updateBoomAnimation(float deltaTime)
{
    boomTimer += deltaTime;

    if (boomTimer >= BOOM_DURATION)
    {
        boomStarted = false;
        boomTimer = 0.0f;
    }
}

// ============================================================================
// RESET FUNCTION
// ============================================================================

void resetGame()
{
    // Reset pillar opacity
    for (int i = 0; i < 4; i++)
    {
        pillarOpacity[i] = 1.0f;
    }

    // Reset game state
    gameTimer = 0.0f;
    gameActive = true;
    gameWon = false;
    gameLost = false;
    gameStatus = "Keep Clicking!";
    boomStarted = false;
    boomTimer = 0.0f;

    // Reset background to white
    bgRed = 1.0f;
    bgGreen = 1.0f;
    bgBlue = 1.0f;

    // Reset component disappearance
    for (size_t i = 0; i < componentDisappeared.size(); i++)
    {
        componentDisappeared[i] = false;
    }

    std::cout << "\n>>> GAME RESET! Start a new round! <<<\n";
}

// ============================================================================
// CREATE BUILDING COMPONENTS - UITS Building Layout
// ============================================================================

void createBuildingComponents(std::vector<BuildingComponent*>& components)
{
    // ========== MAIN BUILDING BODIES ==========
    components.push_back(new BuildingComponent(-3.0f, 0.5f, 2.0f, 5.0f, glm::vec3(0.8f, 0.4f, 0.2f), 0.35f));
    components.push_back(new BuildingComponent(0.0f, 0.2f, 4.4f, 2.0f, glm::vec3(0.8f, 0.4f, 0.2f), 0.4f));
    components.push_back(new BuildingComponent(3.0f, 0.1f, 2.0f, 3.5f, glm::vec3(0.8f, 0.4f, 0.2f), 0.35f));

    // ========== ROOFS ==========
    components.push_back(new BuildingComponent(-3.0f, 2.8f, 2.2f, 0.5f, glm::vec3(0.6f, 0.3f, 0.1f), 0.32f));
    components.push_back(new BuildingComponent(3.0f, 1.95f, 2.2f, 0.5f, glm::vec3(0.6f, 0.3f, 0.1f), 0.32f));

    // ========== PILLARS (INDICES 5-8) ==========
    // Front left pillar
    components.push_back(new BuildingComponent(-1.2f, -1.3f, 0.4f, 1.2f, glm::vec3(0.5f, 0.25f, 0.1f), 0.86f));
    // Front right pillar
    components.push_back(new BuildingComponent(1.2f, -1.3f, 0.4f, 1.2f, glm::vec3(0.5f, 0.25f, 0.1f), 0.87f));
    // Back left pillar
    components.push_back(new BuildingComponent(-0.6f, -1.35f, 0.25f, 1.1f, glm::vec3(0.4f, 0.2f, 0.08f), 0.88f));
    // Back right pillar
    components.push_back(new BuildingComponent(0.6f, -1.35f, 0.25f, 1.1f, glm::vec3(0.4f, 0.2f, 0.08f), 0.89f));

    // ========== BUILDING 1 WINDOWS ==========
    components.push_back(new BuildingComponent(-3.4f, 1.5f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.5f));
    components.push_back(new BuildingComponent(-3.0f, 1.5f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.52f));
    components.push_back(new BuildingComponent(-3.4f, 0.5f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.54f));
    components.push_back(new BuildingComponent(-3.0f, 0.5f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.56f));

    // ========== BUILDING 3 WINDOWS ==========
    components.push_back(new BuildingComponent(3.0f, 1.0f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.5f));
    components.push_back(new BuildingComponent(3.4f, 1.0f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.52f));
    components.push_back(new BuildingComponent(3.0f, 0.0f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.54f));
    components.push_back(new BuildingComponent(3.4f, 0.0f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.56f));

    // ========== BUILDING 2 WINDOWS ==========
    components.push_back(new BuildingComponent(-1.2f, 0.7f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.6f));
    components.push_back(new BuildingComponent(0.0f, 0.7f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.62f));
    components.push_back(new BuildingComponent(1.2f, 0.7f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.64f));
    components.push_back(new BuildingComponent(-1.2f, -0.1f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.65f));
    components.push_back(new BuildingComponent(0.0f, -0.1f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.67f));
    components.push_back(new BuildingComponent(1.2f, -0.1f, 0.35f, 0.35f, glm::vec3(0.2f, 0.5f, 0.8f), 0.69f));

    // ========== WHITE GATE ==========
    components.push_back(new BuildingComponent(0.0f, -1.95f, 3.2f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.8f));
    components.push_back(new BuildingComponent(-1.3f, -1.95f, 0.15f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.81f));
    components.push_back(new BuildingComponent(-0.6f, -1.95f, 0.15f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.82f));
    components.push_back(new BuildingComponent(0.0f, -1.95f, 0.15f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.83f));
    components.push_back(new BuildingComponent(0.6f, -1.95f, 0.15f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.84f));
    components.push_back(new BuildingComponent(1.3f, -1.95f, 0.15f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f), 0.85f));
}
