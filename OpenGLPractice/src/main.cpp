#include <iostream>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShader1Source = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";


void framebuffer_size_callback(GLFWwindow* window, int width, int height) { //Set the size of the rendering viewport to match the window size
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { //Get key returns GLFW_PRESS if pressed, GLFW_RELEASE if not pressed (really just a bool)
        glfwSetWindowShouldClose(window, true);
    }
}
int main2() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Setting initial settings for glfw


    GLFWwindow* window = glfwCreateWindow(1600, 900, "LearnOpenGL", NULL, NULL); //Create new window set to variable window
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); //Creates new window and sets to glfwcurrentcontext so all operations are performed on this window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  //Creates a callback that gets called every time window size is changed

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) //Loads up glad os specific function pointers and puts into glfwGetProcAddress
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 1600, 900); //Sets viewport size, different that window size, this is the box that actually renders stuff
                                //This code is techncially openGL, GLAD library just defined it first so thats where it finds it
    glClearColor(.4f, .9f, .2f, 1.0f);


    //Defining the triangles
    float vertices[] = { //We put these in NDC form (between -1 and 1) so that we can just pass through the vertex shader. We define the structure of this array in the glVertexAttribPointer below
        0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left

    };
    // If we don't have indices, openGL will just make a triangle from every 3 points, but if we repeat verticies, we should not store a vertex more than once
    unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
    };

    //We define an element buffer when we define a VAO below

    //Create shader program
    Shader simpleShader("/home/ethan/Documents/VoxelEngine/OpenGLPractice/src/simplevertex.vert", "/home/ethan/Documents/VoxelEngine/OpenGLPractice/src/simplecolor.frag");
    simpleShader.use();

    //Generating VAOs: Purpose of these is that setting a VBO's attribute configuration does NOT get saved if you switch to another VBO and back. Only one is saved at a time in openGL and
    // needs to be reset every time you switch to a different VBO. VAO's allow these states to be saved and therefore does not require you switch back.

    unsigned int VAO;
    glGenVertexArrays(1, &VAO); // Generates 1 VAO
    glBindVertexArray(VAO); //Binds, any call to a vertex array will use this vertex array from now on. Use this to switch which vertex array you're currently using.

    //Putting the vertices in the buffer, this will be placed inside of VAO since that is the currently bound VAO.

    unsigned int VBO; //The Vertex Buffer Object allows us to send large amounts of vertices to the GPU at a time speeding things up
    glGenBuffers(1, &VBO); //Using VBO as an address and 1 as the number of VBOs to generate
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //now any calls we make to the ARRAY_BUFFER will be using the VBO defined address (there are multiple types of buffers so we must bind)
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Pass in the vertices into the array buffer. Static draw says that we only set data once
    //If we were to have data that changes a lot, like moving vertices, we use GL_DYNAMIC_DRAW for faster writing (This would make sense if this line of code were in a loop)

    //Tell the vertex shader how to interpret our vertex buffer
    //Below is performed on the currently bound VBO & VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // Vertex attribute (position, color, etc). We specify attribute 0 here, matches location 0 in shader meaning position
    // Size of vertex attribute, 3 bytes for vec3
    // type of data this attribute, GL_FLOAT data for a vec3
    // stride, or how far apart each instance of this vertex attrubte is. Here we are only storing location, so every position is 3 * sizeof(float) from each other
    // offset of the first instance of this attribute, here we only have location so we start at 0
    glEnableVertexAttribArray(0); //Enable above attribute configuration, location 0 since that's what we define position attribute as

    //We create an element buffer object to store the incices defined above (this gets attached to the VAO)
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //render loop
    while (!glfwWindowShouldClose(window)) //On a loop, get the pixel buffer and swap with current on, and check for mouse/keybaord input
    {
        //input
        processInput(window); //Similar to pollevents but this can be customed while pollevents has predefined checks/callbacks

        //rendering commands
        glClear(GL_COLOR_BUFFER_BIT); //Clears the frame with the color specifed in glClearColor above
        glBindVertexArray(VAO); //Use the currently bound VAO as the vertices
        //glDrawArrays(GL_TRIANGLES, 0, 6); // Draw triangles using the current shader program! 0 representing starting index of vertex array, and 6 is how many vertexes we want to use
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Wireframe mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Filled mode
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); //Draw triangles using the indices specified in the EBO. 6 indices, GL_UNSIGNED_INT is the type for indicies, and we don't want to offset the EBO

        //events and swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents(); //Calls callback methods, updates windows state
    }

    glfwTerminate();
    return 0;
}




