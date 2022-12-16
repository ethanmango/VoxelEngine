#include <cstdio>
#include <GL/glew.h> //Function pointers
#include <GLFW/glfw3.h> //Window manager
#include "Shader.h"
#include <glm/glm.hpp> //3D math library
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
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
struct Ray {
    vec3 origin;
    vec3 direction;
};

struct cameraInfo {
    vec4 cameraPos;
    vec4 cameraDir;
    vec4 cameraUp;
    vec4 cameraRight;
};
struct clumpInfo {
    //One unit is one voxel, always
    vec4 dimensions;
    mat4 transformationMatrix;
};

//void test(clumpInfo clump, cameraInfo cam, glm::vec2 pixel) {
//    Ray camRay;
//
//
//
//    camRay.origin = cam.cameraPos;
//
//    vec2 resolution = vec2(1920, 1080);
//
//    vec2 aspectRatio = vec2(1.77777777778f, 1.0f);
//    vec2 uv = (pixel / resolution) * 2.0f - 1.0f;
//    float u = aspectRatio.x * ((float(pixel.x) + 0.5f) / float(resolution.x)) - float(aspectRatio.x) / 2.0f;
//    float v = aspectRatio.y * ((float(pixel.y) + 0.5f) / float(resolution.y)) - float(aspectRatio.y) / 2.0f;
//    uv.x *= aspectRatio.x;
//    float distance = 1.0f; //TODO: Pass this value in
//    camRay.direction = normalize(distance * cam.cameraDir + (u * cam.cameraRight) +
//                                 (v * cam.cameraUp));
//
//    //For the formula from the textbook txmin = (xmin - xe)/xd, we write xd as dot(raydir, right) and xe as dot(right, delta), delta being center of BB - ray origin
//    //When right = aligned x axis, this is equal to saying get the x component of the origin point and direction vector, but we're now saying get x component on a rotated axis
//    //Hard coding for now
////    vec3 right = vec3(1,0,0);
////    vec3 up = vec3(0,1,0);
////    vec3 forward = vec3(0,0, 1);
//
//    mat4 transformMat = clump.transformationMatrix;
//    vec3 right = transformMat[0];
//    vec3 up = transformMat[1];
//    vec3 forward = transformMat[2];
//    vec3 translate = transformMat[3];
//
//    float xMax = clump.BBmaxPoint.x;
//    float yMax = clump.BBmaxPoint.y;
//    float zMax = clump.BBmaxPoint.z;
//
//    float xMin = clump.BBminPoint.x;
//    float yMin = clump.BBminPoint.y;
//    float zMin = clump.BBminPoint.z;
//
//
//    bool intersects;
//    float epsilon = 0.001f;
//    vec3 delta = translate - camRay.origin;
//    //These calculations need to be positive in the corrosponding axis (right is x)
//
//    float deltaX = dot(right, delta);
//    float unalignedDirX = dot(right, camRay.direction);
//
//    float deltaY = dot(up, delta);
//    float unalignedDirY = dot(up, camRay.direction);
//
//    float deltaZ = dot(forward, delta);
//    float unalignedDirZ = dot(forward, camRay.direction);
//
//
//    float a_x = 1.0f / unalignedDirX;
//    float a_y = 1.0f / unalignedDirY;
//    float a_z = 1.0f / unalignedDirZ;
//
//    float t_min_x;
//    float t_min_y;
//    float t_min_z;
//
//    float t_max_x;
//    float t_max_y;
//    float t_max_z;
//
//    //Reasoning for these checks. If you enter any of the 3 bounds (x,y,z) and then leave any of them
//    //if you are intersecting, you must have entered all 3 already, so once you leave, you can't enter any again
//    //if you enter again after leaving any of the bounds, you must have been outside of the box to begin with
//
//
//    vec3 normal;
//    bool enteredXPos;
//    bool enteredYPos;
//    bool enteredZPos;
//
//    if (a_x >= 0) {
//        enteredXPos = true;
//        t_min_x = a_x * (xMin + deltaX);
//        t_max_x = a_x * (xMax + deltaX);
//    } else {
//        enteredXPos = false;
//        t_max_x = a_x * (xMin + deltaX);
//        t_min_x = a_x * (xMax + deltaX);
//    }
//
//    if (a_y >= 0) {
//        enteredYPos = true;
//        t_min_y = a_y * (yMin + deltaY);
//        t_max_y = a_y * (yMax + deltaY);
//    } else {
//        enteredYPos = false;
//        t_max_y = a_y * (yMin + deltaY);
//        t_min_y = a_y * (yMax + deltaY);
//    }
//
//    if (a_z >= 0) {
//        enteredZPos = true;
//
//        t_min_z = a_z * (zMin + deltaZ);
//        t_max_z = a_z * (zMax + deltaZ);
//    } else {
//        enteredZPos = false;
//        t_max_z = a_z * (zMin + deltaZ);
//        t_min_z = a_z * (zMax + deltaZ);
//    }
//
//    bool enteredXFirst = false;
//    bool enteredYFirst = false;
//    bool enteredZFirst = false;
//    if (t_min_y > t_min_z && t_min_y > t_min_x){
//        enteredYFirst = true;
//    }
//    else if (t_min_x > t_min_z && t_min_x > t_min_y){
//        enteredXFirst = true;
//    }
//    else {
//        enteredZFirst = true;
//    }
//
//    float t_min = max(t_min_x, max(t_min_y, t_min_z));
//    float t_max = min(t_max_x, min(t_max_y, t_max_z));
//
//    if (t_min <= t_max) {
//        if (t_min > 100000) {
//            intersects = false;
//            return;
//        } else if (t_max < 0) {
//            intersects = false;
//        } else intersects = true;
//    } else {
//        intersects = false;
//    }
//
//
//    if (intersects){
//        //If we get here, we know that the ray intersects the bounding box, and
//        // we can check the intersection point as being t_min
//        //Now we must check if it hits any voxels in the BB.
//
//        mat4 rotationMat = mat4(transformMat[0], transformMat[1], transformMat[2], vec4(0,0,0,1));
////        if (rotationMat[2] == vec3(0,0,1)){
////                FragColor = vec4(.21f, .1f, .23f, 1.0f);
////                return;
////            }
////            else {
////                FragColor = vec4(0,0,0, 1.0f);
////                return;
////            }
//        vec3 intersectionPoint = camRay.origin + (t_min * camRay.direction);
//
//        //Only need to rotate the cam origin and direction, as t_min takes care of the translation
//        vec3 translatedCamOrigin = camRay.origin - translate;
//        vec3 translatedCamDir = camRay.direction - translate;
//
//        vec3 rotatedCamOrigin = (inverse(rotationMat) * vec4(translatedCamOrigin,0));
//        rotatedCamOrigin = rotatedCamOrigin + translate;
//
//        vec3 rotatedDirection = (inverse(rotationMat) * vec4(translatedCamDir,0));
//        rotatedDirection = normalize(rotatedDirection + translate + camRay.origin);
//
//        vec3 transformedIntersectionPoint = rotatedCamOrigin + (t_min * rotatedDirection);
//        vec3 normal;
//        //If we get here, we know that the ray intersects the bounding box, and
//        // we can check the intersection point as being t_min
//        //Now we must check if it hits any voxels in the BB.
//        if (abs(intersectionPoint.x - xMin) < epsilon) {
//            normal = -1.0f * right;
//        } else if (abs(intersectionPoint.x - xMax) < epsilon) {
//            normal = right;
//
//        } else if (abs(intersectionPoint.y - yMin) < epsilon) {
//            normal = -1.0f * up;
//        } else if (abs(intersectionPoint.y - yMax) < epsilon) {
//            normal = up;
//        } else if (abs(intersectionPoint.z - zMin) < epsilon) {
//            normal = -1.0f * forward;
//        } else {
//            normal = forward;
//        }
//
//
//        //Below is a point light, if we want a directional light, the l is simply a unit vector pointing towards
//        // that directional light
//        vec3 lightPos = vec3(5.0f, 5.0f, 5.0f);
//        vec3 boxAmbientCoff = vec3(0.8f, 0.2f, 0.3f);
//        float lightAmbientIntensity = 0.2f;
//        vec3 ambientShade = boxAmbientCoff * lightAmbientIntensity;
//
//
//        vec3 boxDiffuseCoff = vec3(0.8f, 0.2f, 0.3f);
//        float lightIntensity = 0.8f;
//        vec3 l = normalize(lightPos - intersectionPoint);
//        vec3 lambertionShade = boxDiffuseCoff * lightIntensity * max(0.0f, dot(normal, l));
//
//        vec3 boxSpecularCoff = vec3(1.0f, 1.0f, 1.0f);
//        vec3 v = normalize(camRay.origin - intersectionPoint);
//        vec3 halfVector = (v + l) / length(v + l);
//        float phongExponent = 10.0f;
//        vec3 specularShader = boxSpecularCoff * lightIntensity * pow(max(0.0f, dot(normal, halfVector)), phongExponent);
//
//        //Now we can do basic lighting on the cube (no ray traced lighting since we only have one object)
//    }
//}
// Can still use AABB for making a Binary Hierarchy later on, but the game world isn't aligned to the grid,
// only voxels within a clump are aligned to their own grid, so AABB isn't that useful for rendering or collision

//TODO: Call when something is animating rather than only at initialization or at every frame.

//Pass in current right, up, forward normals, and the angle difference from the previous frame.
//So if we want to add 1 degree to x, angleDiffs.x = 1. That way we only adjust the normals
//for the angles that moved. Actual current angle and difference calculations done somewhere else.
void recalculateClumpNormals(glm::vec3 angleDiffs, glm::mat3 *normalsMat){

    float angleX = angleDiffs.x;
    float angleY = angleDiffs.y;
    float angleZ = angleDiffs.z;
    glm::mat3 rotationX, rotationY, rotationZ;

    //Only bother calculating rotation if rotation isn't 0)
    if (angleX != 0){
        rotationX = glm::mat3(1,0,0,0,cos(angleX), sin(angleX),0,-sin(angleX), cos(angleX));
        //Rotation on X axis means rotating "up" and "forward" vectors
        normalsMat[1] = rotationX * normalsMat[1];
        normalsMat[2] = rotationX * normalsMat[2];
    }

    if (angleY != 0){
        rotationY = glm::mat3(cos(angleY),0,-sin(angleY),0,1, 0,sin(angleY),0, cos(angleY));
        //Rotation on Y axis means rotating "right" and "forward" vectors
        normalsMat[0] = rotationY * normalsMat[0];
        normalsMat[2] = rotationY * normalsMat[2];
    }

    if (angleZ != 0){
        rotationZ = glm::mat3(cos(angleZ),sin(angleZ),0,-sin(angleZ),cos(angleZ), 0,0,0, 1);
        //Rotation on Z axis means rotating "right" and "up" vectors
        normalsMat[0] = rotationZ * normalsMat[0];
        normalsMat[1] = rotationZ * normalsMat[1];
    }



}


int main(){

    //First we initialize GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit()){
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    float targetFrameRate = 300.0f;
    glfwWindowHint(GLFW_SAMPLES, 4); //Defines 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); //Use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Don't use old OpenGL

    //Create a window
    int width, height;
    width = 1920;
    height = 1080;

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
    //Here we just have 2 triangles to cover the screen
    float b1VerticesBuffer[] = {
            -1,-1,0,
            1,-1,0,
            1, 1,0,
            -1,1,0
    };
    unsigned int b1boxIndices[] = {
        0,1,2,
        2,3,0
    };

    //We create an element buffer object to store the indices defined above (this gets attached to the VAO)
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
//    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
//    GLuint CameraPosID = glGetUniformLocation(programID, "cameraPos");
//    GLuint CameraDirID = glGetUniformLocation(programID, "cameraDir");
//    GLuint CameraUpID = glGetUniformLocation(programID, "cameraUp");
//    GLuint CameraRightID = glGetUniformLocation(programID, "cameraRight");
    //GLuint CameraInfoID = glGetUniformLocation(programID, "cameraInfo");


    glEnable(GL_CULL_FACE); //Turns off seeing "inside" meshes
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    double lastTime = glfwGetTime();
    double frameLimitTime = glfwGetTime();
    setCameraPos(vec3(0,0,10));
//    setCameraDir(vec3(0,2,-1));
    int nbFrames = 0;
    glClearColor(.71f, .5f, .93f, 1.0f);


    //Initial camera and clump data. These are passed in every frame into the shader, so can be updated on a loop
    //or simply whenever they change in the scene. (loop can use a getter to grab updated information).

    //Calculate initial rotation
    glm::mat3 initialClumpNormals = glm::mat3(glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1));

    // In future set up clump struct that has methods such as rotateByDegrees and getClumpNormals
    // RotateByDegrees, for example, will take in the degree difference and call recalculateClumpNormals
    // while setRotation can set the rotation directly (and calculate angle differences).
    recalculateClumpNormals(glm::vec3(45,0,0), &initialClumpNormals);


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
        //Note: For now we're not using MVP because we're raytracing and using two large triangles as fragments

        //glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
//        glUniform3f(CameraPosID, CameraPos.x, CameraPos.y, CameraPos.z);
//        glUniform3f(CameraDirID, CameraDir.x, CameraDir.y, CameraDir.z);
//        glUniform3f(CameraRightID, CameraRight.x, CameraRight.y, CameraRight.z);
//        glUniform3f(CameraUpID, CameraUp.x, CameraUp.y, CameraUp.z);

        cameraInfo frameCamInfo;
        frameCamInfo.cameraPos = vec4(CameraPos,0);
        frameCamInfo.cameraDir = vec4(CameraDir,0);
        frameCamInfo.cameraRight = vec4(CameraRight,0);
        frameCamInfo.cameraUp = vec4(CameraUp,0);
        struct cameraInfo frameCamInfoArr[1];
        frameCamInfoArr[0] = frameCamInfo;

        clumpInfo frameClumpInfo;
        frameClumpInfo.dimensions = vec4(1,1,1,0);
        vec4 translate = vec4(vec3(0,0,0),1);

        mat4 frameTransformMat = mat4(vec4(initialClumpNormals[0], 0), vec4(initialClumpNormals[1],0), vec4(initialClumpNormals[2],0), translate);

        frameClumpInfo.transformationMatrix = frameTransformMat;
        struct clumpInfo frameClumpInfoArr[1];
        frameClumpInfoArr[0] = frameClumpInfo;
//        for (int i = 0; i < 1920; i++) {
//            for (int j = 0; j < 1080; j++) {
               // test(frameClumpInfo, frameCamInfo, glm::vec2(5,5));
//            }
//        }
        GLuint frameCameraSSBO;
        glGenBuffers(1, &frameCameraSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, frameCameraSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(frameCamInfoArr), frameCamInfoArr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, frameCameraSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        GLuint clumpArrSSBO;
        glGenBuffers(1, &clumpArrSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, clumpArrSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(frameClumpInfoArr), frameClumpInfoArr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, clumpArrSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT);

        //Draw the triangle
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Filled mode
        glDrawElements(GL_TRIANGLES, sizeof(b1boxIndices), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //Frame rate limiter with 300 fps target
        while (glfwGetTime() < frameLimitTime + 1.0/targetFrameRate) {
           //Do nothing here, just waiting till loop ends to limit framerate
        }
        frameLimitTime += 1.0/targetFrameRate;

    }
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}


