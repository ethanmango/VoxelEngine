#include <cstdio>
#include <GL/glew.h> //Function pointers
#include <GLFW/glfw3.h> //Window manager
#include "Shader.h"
#include <glm/glm.hpp> //3D math library
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
#include "controls.h"

int main(){

    //First we initialize GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit()){
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); //Defines 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); //Use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Don't use old OpenGL

    //Create a window
    int width, height;
    width = 1024;
    height = 728;
    GLFWwindow* window = glfwCreateWindow(width, height, "Tutorial 1", nullptr, nullptr);
    if (window == nullptr){
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    //Set the current openGL context to newly created window
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK){
        fprintf(stderr, "Failed to initialize GLEW.\n");
        return -1;
    }

    //Here we define the vertices using a VAO (purpose is to be able to switch between buffers without resetting, only done once)
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //Now we can define the vertices in screen space (-1 to 1 x and y) defined as 3 points on the screen
    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
    };

    //Need to generate the MVP matrix (model view projection) to convert from local to world to camera to clip space
    //glm::mat4 World = glm::mat4(1.0f); //Model matrix is just identity, we can represent vertices in world space to begin with
                                          //or otherwise convert into world space using predefined rules with how we import objects
                                          // (like if each object has a defined center at 0,0,0 and vertices are defined with respect to that)
                                          // Note this only works if we use a different VAO for each object, so usually this is just identity
                                          // So for now I won't include it in MVP calculation as to save calculation time

    //glm::lookAt generates the transform*rotation*scale matrix to move all objects from world space to camera space
    // so that the camera will now be at position 0,0,0 aimed in the direction of the second argument
    //glm::mat4 View = glm::lookAt(
//            glm::vec3(4,3,3), //Position of camera in world space
//            glm::vec3(0,0,0), //Direction of camera
//            glm::vec3(0,1,0) //Orientation (head is up)
//            );

    //Defines FOV (45 degrees here), width and height aspect ratio, and near and far clipping planes
    //glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float) width / (float)height, 0.1f, 100.0f);
    //Creates the projection matrix to convert from world space to clip space (reformat to screen size as well as projects
    //z position in order to add perspective depth rather than orthographic)



    //Generate a new vertex buffer and insert the data from the defined vertices
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    //TODO: for now, will keep everything in same VAO which means both voxel AND nonvoxel need UV data. We simply
    // won't bind anything for the voxels as we won't read it. Maybe in the future we can separate voxel and not voxel
    //  so the voxel attributes are only what we need.

    //Set vertex buffer attributes
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    //The attribute is our definition of what a "vertex" means. In this case, we have 3 floats per vertex, and
    //Set this to attribute 0. So in a vertex shader, we can grab attribute 0 and know it will be 3 floats
    //representing a point in 3D. We can add attributes like color, for example.
    glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)nullptr            // array buffer offset
    );

    //Generate projection matrix, if width and height can change, need to recalculate this
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float) width / (float) height, 0.1f, 100.0f);

    //Call our shaders
    Shader newShader = Shader();
    GLuint programID = newShader.LoadShaders( "/home/ethan/Documents/VoxelEngine/OpenGLPractice/src/simplevertex.vert", "/home/ethan/Documents/VoxelEngine/OpenGLPractice/src/simplecolor.frag");
    glUseProgram(programID);

    //Pass in the MVP matrix calculated above to vertex shader, first we generate a location for it and label it MVP
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    //Need to capture escape key in order to quit window
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    //FPS counter setup
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    //On a loop clear the buffer, swap the buffers, then poll for events (call callback methods)
    do {
        //FPS Counter
        // Measure speed
        double currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
            // printf and reset timer
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
            printf("%f FPS\n", double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }

        //Recalculate view and projection matrix based on keyboard and mouse input
        computeMatricesFromInputs(window, width, height);
        glm::mat4 View = getViewMatrix();
        glm::mat4 MVP = Projection * View;         //We now have our MVP matrix we can pass into vertex shader, we can also
                                                   // pass in just View * World to get the camera space projection for ray tracing

        //Then we can pass in the MVP matrix into the generated matrixID, if M changes per model this needs to be in the loop
        // (only relevant if using multiple VAO, like a different shader and group of vertices for each object)
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);


        glClear(GL_COLOR_BUFFER_BIT);

        //Draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}



