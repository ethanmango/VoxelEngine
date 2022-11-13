#include <cstdio>
#include <GL/glew.h> //Function pointers
#include <GLFW/glfw3.h> //Window manager
#include "Shader.h"
#include <glm/glm.hpp> //3D math library
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
#include "controls.h"

//TODO: Plan, hardcode bounding boxes, then hardcode voxel positions and colors, then generate both from .vox files when
//  ray tracing algorithm is working
//  Need to figure out how to get normal of a voxel based on the ray trace, since each voxel has 6 normals depending
//  on where the ray hits. We then multiply this unit normal by the orientation (rotation) of the bounding box to get the normal
//  of the face of the voxel that was hit
//  Should prob calculate the rotation of the bounding box in the CPU since its the same for each pixel, we will just need
//  to pass in all the bounding box rotations and when we ray trace and hit a bounding box, we can identify and grab the
//  corresponding rotation
//  Pass in array of bounding box vertices (all 8 vertices) as well as bounding box rotations in same order, then
//  if we find a bounding box match, we simply get the bounding box rotation from the other uniform at the same index
//  recalculating for each pixel doesn't make sense

// TODO: Next step is to ray trace just the bounding box, not the voxels inside. Need to check if a ray intersects
//  with an OOB since the bounding box can rotate, but once we found a ray hit, we can transform the ray into object
//  space (as if its aligned) and do an aligned method to find the needed voxel
//  http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
//  Normal can be found by seeing which plane you entered. minx, maxX, y, z. Entered plane determines normal
//  since OOB is not aligned the normal we get from this doesn't need to be rotated.
//  Might not even need to translate to AABB, since we are prob checking the min max planes, which are determined by
//  the OOB planes + an offset

// Can still use AABB for making a Binary Hierarchy later on, but the game world isn't aligned to the grid,
// only voxels within a clump are aligned to their own grid, so AABB isn't that useful for rendering or collision
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
    float b1VerticesBuffer[] = {
//        0.0f, 0.0f, 1.0f, //bottom left front
//        0.0f, 0.0f, 0.0f, //bottom left back
//        1.0f,  0.0f, 1.0f, //bottom right front
//        1.0f, 0.0f, 0.0f, //bottom right back
//        0.0f, 1.0f, 1.0f, //top left front
//        0.0f,  1.0f, 0.0f, //top left back
//        1.0f, 1.0f, 1.0f, //top right front
//        1.0f,  1.0f, 0.0f, //top right back
            -1,-1,0,
            1,-1,0,
            1, 1,0,
            -1,1,0
    };
    unsigned int b1boxIndices[] = {
//      0,1,2,
//      2,1,3, //Bottom face
//      6,4,0,
//      2,6,0, //Front face
//     7,6,2,
//     7,2,3, //Right face
//     4,5,0,
//     0,5,1, //Left face
//      5,4,6,
//      5,6,7, //Top face
//      1,5,7,
//      1,7,3 // Back face
        0,1,2,
        2,3,0
    };

    //We create an element buffer object to store the incices defined above (this gets attached to the VAO)
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(b1boxIndices), b1boxIndices, GL_STATIC_DRAW);

    //Generate a new vertex buffer and insert the data from the defined vertices
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(b1VerticesBuffer), b1VerticesBuffer, GL_STATIC_DRAW);

    //Set vertex buffer attributes
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
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
    GLuint programID = newShader.LoadShaders( "/home/ethan/Documents/VoxelEngine/OpenGLVoxel/src/voxel.vert", "/home/ethan/Documents/VoxelEngine/OpenGLVoxel/src/raytracevoxel.frag");
    glUseProgram(programID);

    //Pass in the MVP matrix calculated above to vertex shader, first we generate a location for it and label it MVP
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint CameraPosID = glGetUniformLocation(programID, "cameraPos");
    GLuint CameraDirID = glGetUniformLocation(programID, "cameraDir");
    GLuint CameraUpID = glGetUniformLocation(programID, "cameraUp");
    GLuint CameraRightID = glGetUniformLocation(programID, "cameraRight");


    glEnable(GL_CULL_FACE); //Turns off seeing "inside" meshes
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    double lastTime = glfwGetTime();
    int nbFrames = 0;
    glClearColor(.71f, .5f, .93f, 1.0f);
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
        glm::vec3 CameraPos = getCameraPos();
        glm::vec3 CameraDir = getCameraDir();
        glm::vec3 CameraRight = getCameraRight();
        glm::vec3 CameraUp = getCameraUp();
        glm::mat4 MVP = Projection * View;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniform3f(CameraPosID, CameraPos.x, CameraPos.y, CameraPos.z);
        glUniform3f(CameraDirID, CameraDir.x, CameraDir.y, CameraDir.z);
        glUniform3f(CameraRightID, CameraRight.x, CameraRight.y, CameraRight.z);
        glUniform3f(CameraUpID, CameraUp.x, CameraUp.y, CameraUp.z);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);


        glClear(GL_COLOR_BUFFER_BIT);

        //Draw the triangle
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Filled mode
        glDrawElements(GL_TRIANGLES, sizeof(b1boxIndices), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}



