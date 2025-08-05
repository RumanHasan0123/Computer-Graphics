// NAME: Gazi Ruman Hasan
// ID: 043231000101065
#include "glad.h"
#include "glfw3.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;




// shader sources
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan\n"
    "}\n\0";


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "RumanHasan", NULL, NULL);//My name is Ruman hasan , so i named the window RumanHasan.
     //Resizable or not, multiple window sharing?

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window); //following all commands on this object, must call this function after glfwCreateWindow() before issuing any OpenGL commands.

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //update when the user stretches the window

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    



     // Compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex Shader Compilation Failed\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment Shader Compilation Failed\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader Program Linking Failed\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Define square (two triangles) + triangle on top
    float vertices[] = {
        // Square (two triangles)
        -0.5f, -0.5f, 0.0f,  // Bottom left
         0.5f, -0.5f, 0.0f,  // Bottom right
         0.5f,  0.5f, 0.0f,  // Top right

        -0.5f, -0.5f, 0.0f,  // Bottom left
         0.5f,  0.5f, 0.0f,  // Top right
        -0.5f,  0.5f, 0.0f,  // Top left

        // Triangle on top (shares top-left and top-right of square)
        -0.5f,  0.5f, 0.0f,  // Top left
         0.5f,  0.5f, 0.0f,  // Top right
         0.0f,  0.9f, 0.0f   // Top center (peak of triangle)
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 


    // render loop
    // -----------
    //This block of code is the main loop that runs continuously until the window is closed.
    while (!glfwWindowShouldClose(window)) //checks whether the user has clicked the close button on the window or triggered any event to close it.
    {
        // input
        // -----
        processInput(window); //handle keyboard or mouse input.


        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // White  Color er Background er jonno RGB = (0, 1, 1) disi
        glClear(GL_COLOR_BUFFER_BIT);


        // Render
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 9); // 6 for square + 3 for triangle

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window); //This swaps the back buffer (where your latest OpenGL drawing happens) with the front buffer (the one displayed on screen).
        //Imagine you're drawing a picture. You draw it on a piece of paper that no one can see, and once you're done, you quickly swap it with the paper on the table that everyone is looking at. The result is a smooth transition, with no one seeing the process of you drawing.

        glfwPollEvents(); //respond to user actions like pressing a key, moving the mouse, or resizing the window.
    }


    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);


    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) 
{
    if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)//Window will be closed if The Key "R"(First Letter Of My name) pressed //close the window when the user pressed escape
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays. 
    glViewport(0, 0, width, height);
}