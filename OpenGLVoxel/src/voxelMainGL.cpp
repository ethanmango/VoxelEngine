#include <cstdio>
#include <GL/glew.h> //Function pointers
#include <GLFW/glfw3.h> //Window manager
#include "Shader.h"
#include "readVox.h"
#include <glm/glm.hpp> //3D math library
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
using namespace glm;
#include "controls.h"






//TODO: KNOWN ISSUES: COLOR IS OFF (BLUISH TINT), memory leak when high number of voxels (memory usage keeps going up)
// rust should fix this if its a real memory leak issue... camera inside clump causes WEIRD stuff to happen... this is
// definetly just something wrong with the shader code.

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
    vec4 colors[256];
};


//bool voxelExists(vec3 dimensions, int x, int y, int z){
//    if (x < 0 || y < 0 || z < 0){
//        return false;
//    }
//    if (x >= dimensions.x || y >= dimensions.y || z >= dimensions.z){
//        return false;
//    }
//    return true;
//
//}
//int existenceCoords(vec3 dims, int x, int y, int z) {
//    return z * int(dims.y) * int(dims.x) + y * int(dims.x) + x;
//}
//bool searchClump(vec3 clumpIntersectionPoint, vec3 dir, vec3 foundIntersectionPoint, vec3 dims, vec3 center){
//    //Using similar strategy as in BB tracing we can multiply by reciprical rather than divide as to avoid -0 value.
//    //Need to make sure any time we are using this t value though, we are using it for comparisons.
//    glm::vec3 clumpDims = dims;
//    glm::vec3 clumpCenter = center;
//
//    int voxelExistence[8] = {1,1,1,1,1,1,-1,1};
//
//    //First we need to check which voxel our clumpintersectionPoint exists at.
//    float minBoundX = clumpCenter.x - (clumpDims.x / 2.0f);
//    int xVoxelCoord = max(0,int(floor(clumpIntersectionPoint.x - minBoundX)));
//    if (xVoxelCoord > int(clumpDims.x) - 1) xVoxelCoord = int(clumpDims.x) - 1;
//
//    float minBoundY = clumpCenter.y - (clumpDims.y / 2.0f);
//    int yVoxelCoord = max(0,int(floor(clumpIntersectionPoint.y - minBoundY)));
//    if (yVoxelCoord > int(clumpDims.y) - 1) yVoxelCoord = int(clumpDims.y) - 1;
//
//
//    float minBoundZ = clumpCenter.z - (clumpDims.z / 2.0f);
//    int zVoxelCoord = max(0,int(floor(clumpIntersectionPoint.z - minBoundZ)));
//    if (zVoxelCoord > int(clumpDims.z) - 1) zVoxelCoord = int(clumpDims.z) - 1;
//
//    int existenceIndex = existenceCoords(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord);
//
//
//    //Return early if the starter voxel exists
//    if (voxelExistence[existenceIndex] != -1) {
//        foundIntersectionPoint = clumpIntersectionPoint;
//        return true;
//    };
//
//    //Now that we have the voxel that we start in, we can start the traversal algorithm
//    ///First we get the step direction
//    glm::vec3 stepDir;
//    if (dir.x >= 0) stepDir.x = 1;
//    else stepDir.x = -1;
//    if (dir.y >= 0) stepDir.y = 1;
//    else stepDir.y = -1;
//    if (dir.z >= 0) stepDir.z = 1;
//    else stepDir.z = -1;
//
//    //Next we get the tDelta, or how far along the ray we need to go to cross an entire voxel
//    vec3 tDelta;
//    //Need to take absolute value as delta is a scalar and direction doesn't matter
//    tDelta.x = abs(1.0f/dir.x);
//    tDelta.y = abs(1.0f/dir.y);
//    tDelta.z = abs(1.0f/dir.z);
//
//    //Now we initalize tMax, which is how far along the ray to get to a certain voxel in each direction.
//    //These all start out as how far until the first next voxel, then get tDelta added to it each time after
//
//    vec3 tMax;
//    vec3 comparisonLine;
//
//    comparisonLine.x = minBoundX + xVoxelCoord;
//    comparisonLine.y = minBoundY + yVoxelCoord;
//    comparisonLine.z = minBoundZ + zVoxelCoord;
//
//    if (stepDir.x == 1) comparisonLine.x = comparisonLine.x + 1;
//    if (stepDir.y == 1) comparisonLine.y = comparisonLine.y + 1;
//    if (stepDir.z == 1) comparisonLine.z = comparisonLine.z + 1;
//
//    //Abs value here again as t is a scalar and direction doesn't matter
//    tMax.x = tDelta.x * abs(comparisonLine.x - clumpIntersectionPoint.x);
//    tMax.y = tDelta.y * abs(comparisonLine.y - clumpIntersectionPoint.y);
//    tMax.z = tDelta.z * abs(comparisonLine.z - clumpIntersectionPoint.z);
//
//    float closestTMax = 0;
//    int closestTMaxAxis = 1;    //Now we are done with the initialization step, and can move onto the iterative.
//    while (voxelExists(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord)){
//
//        //Need to see if it exists first before we increment in case we go out of bounds
//        existenceIndex = existenceCoords(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord);
//        if (voxelExistence[existenceIndex] != -1 && closestTMax != 0) {
//            foundIntersectionPoint = clumpIntersectionPoint + closestTMax * dir;
//
//            return true;
//        };
//
//        if (tMax.x <= tMax.y && tMax.x <= tMax.z){
//            xVoxelCoord += int(stepDir.x);
//            closestTMax = tMax.x;
//            tMax.x += tDelta.x;
//            closestTMaxAxis = 0;
//
//        }
//        else if (tMax.y <= tMax.x && tMax.y <= tMax.z){
//            yVoxelCoord += int(stepDir.y);
//            closestTMax = tMax.y;
//            tMax.y += tDelta.y;
//            closestTMaxAxis = 1;
//
//        }
//        else {
//            zVoxelCoord += int(stepDir.z);
//            closestTMax = tMax.z;
//            tMax.z += tDelta.z;
//            closestTMaxAxis = 2;
//
//        }
//    }
//
//    return false;
//
//
//}
//
//vec3 rotatePointAroundPoint(vec3 pointToRotate, vec3 rotationCenter, mat3 rotationMatrix, bool inverseRotation){
//    vec3 translatedPoint = pointToRotate - rotationCenter;
//    vec3 rotatedIntersectionPoint;
//
//    if (inverseRotation){
//        rotatedIntersectionPoint = inverse(rotationMatrix) * translatedPoint;
//    }
//    else {
//        rotatedIntersectionPoint = rotationMatrix * translatedPoint;
//    }
//    return rotatedIntersectionPoint + rotationCenter;
//}
//
//vec3 rotateDirectionAroundPoint(vec3 directionToRotate, vec3 rotationCenter, mat3 rotationMatrix, bool inverseRotation){
//    vec3 translatedDir = directionToRotate - rotationCenter;
//    vec3 translatedOrigin = -rotationCenter;
//    vec3 rotatedOrigin = inverse(rotationMatrix) * translatedOrigin;
//    rotatedOrigin = rotatedOrigin + rotationCenter;
//
//    vec3 rotatedDir;
//    if (inverseRotation){
//        rotatedDir = inverse(rotationMatrix) * translatedDir;
//    }
//    else {
//        rotatedDir = rotationMatrix * translatedDir;
//    }
//    return rotatedDir + rotationCenter - rotatedOrigin;
//}
//
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
////    mat4 transformMat = clump.transformationMatrix;
////    vec3 right = transformMat[0];
////    vec3 up = transformMat[1];
////    vec3 forward = transformMat[2];
////    vec3 translate = transformMat[3];
////
////    float xMax = clump.BBmaxPoint.x;
////    float yMax = clump.BBmaxPoint.y;
////    float zMax = clump.BBmaxPoint.z;
////
////    float xMin = clump.BBminPoint.x;
////    float yMin = clump.BBminPoint.y;
////    float zMin = clump.BBminPoint.z;
//    mat4 transformMat = clump.transformationMatrix;
//    vec3 right = transformMat[0];
//    vec3 up = transformMat[1];
//    vec3 forward = transformMat[2];
//    vec3 translate = transformMat[3];
//
//    vec3 dimensions = clump.dimensions;
//    float xMax = dimensions.x/2;
//    float yMax = dimensions.y/2;
//    float zMax = dimensions.z/2;
//
//    float xMin = -xMax;
//    float yMin = -yMax;
//    float zMin = -zMax;
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
//
//
//        mat3 rotationMat = mat3(transformMat[0], transformMat[1], transformMat[2]);
//        vec3 intersectionPoint = camRay.origin + (t_min * camRay.direction);
//
//        vec3 worldAlignedIntersectionPoint = rotatePointAroundPoint(intersectionPoint, translate, rotationMat, true);
//        vec3 worldAlignedRayDirection = rotateDirectionAroundPoint(camRay.direction, translate, rotationMat, true);
//        searchClump(worldAlignedIntersectionPoint, worldAlignedRayDirection, vec3(0,0,0), dimensions, translate);
//    }
//}
// Can still use AABB for making a Binary Hierarchy later on, but the game world isn't aligned to the grid,
// only voxels within a clump are aligned to their own grid, so AABB isn't that useful for rendering or collision

//TODO: Call when something is animating rather than only at initialization or at every frame.

//Pass in current right, up, forward normals, and the angle difference from the previous frame.
//So if we want to add 1 degree to x, angleDiffs.x = 1. That way we only adjust the normals
//for the angles that moved. Actual current angle and difference calculations done somewhere else.
void recalculateClumpNormals(glm::vec3 angleDiffs, glm::mat3 *normalsMat){

    //Convert to radians to calculate rotation stuff
    float angleX = angleDiffs.x * (M_PI/180);
    float angleY = angleDiffs.y * (M_PI/180);
    float angleZ = angleDiffs.z * (M_PI/180);
    glm::mat3 rotationX, rotationY, rotationZ;

        rotationX = glm::mat3(1,0,0,0,cos(angleX), sin(angleX),0,-sin(angleX), cos(angleX));
        rotationY = glm::mat3(cos(angleY),0,-sin(angleY),0,1, 0,sin(angleY),0, cos(angleY));
        rotationZ = glm::mat3(cos(angleZ),sin(angleZ),0,-sin(angleZ),cos(angleZ), 0,0,0, 1);


    //ORDER of rotation matters, will change what shape looks like, so we will define it as X first, then Y, then Z;
    //(as long as this is consistent it makes no difference to the user, they just change the angle according to what
    //it currently looks like, its just that those same values would look different in a different order).
    *normalsMat = rotationZ * rotationY * rotationX * *normalsMat;
}


int main(){

    //First we initialize GLFW=+
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

    //Enable transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
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
    GLuint programID = newShader.LoadShaders( "/home/ethanmoran/Documents/VoxelEngine/OpenGLVoxel/src/voxel.vert", "/home/ethanmoran/Documents/VoxelEngine/OpenGLVoxel/src/raytracevoxel.frag");
    glUseProgram(programID);

    //Pass in the MVP matrix calculated above to vertex shader, first we generate a location for it and label it MVP


    //Now we will pass in a vox file to read
    readVox newReadVox = readVox();
    readVox::voxInfo newVoxInfo;
    newVoxInfo = newReadVox.readFromFile("/home/ethanmoran/Documents/VoxelEngine/OpenGLVoxel/src/monu2.vox");


    vec4 colorPalette[256];
    for (int i = 0; i < 255; i++){
        colorPalette[i].x = float (newVoxInfo.allModelsInFile[0].colors[i * 4]) / 255.0f;
        colorPalette[i].y = float (newVoxInfo.allModelsInFile[0].colors[i * 4 + 1]) / 255.0f;
        colorPalette[i].z = float (newVoxInfo.allModelsInFile[0].colors[i * 4 + 2]) / 255.0f;
        colorPalette[i].w = 1.0f;
    }


    //Need to load array into a buffer


    glEnable(GL_CULL_FACE); //Turns off seeing "inside" meshes
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    double lastTime = glfwGetTime();
    double frameLimitTime = glfwGetTime();
    setCameraPos(vec3(0,0,50));
//    setCameraDir(vec3(0,2,-1));
    int nbFrames = 0;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


    //Initial camera and clump data. These are passed in every frame into the shader, so can be updated on a loop
    //or simply whenever they change in the scene. (loop can use a getter to grab updated information).

    //Calculate initial rotation
    glm::mat3 initialClumpNormals = glm::mat3(glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1));

    // In future set up clump struct that has methods such as rotateByDegrees and getClumpNormals
    // RotateByDegrees, for example, will take in the degree difference and call recalculateClumpNormals
    // while setRotation can set the rotation directly (and calculate angle differences).
    recalculateClumpNormals(glm::vec3(0,180,0), &initialClumpNormals);


    clumpInfo frameClumpInfo;
    std::copy(std::begin(colorPalette), std::end(colorPalette), std::begin(frameClumpInfo.colors));
//    std::vector<int> frameVoxelArray = newVoxInfo.allModelsInFile[0].voxelArray;
//    std::copy(frameVoxelArray.begin(), frameVoxelArray.end(), frameClumpInfo.voxInfo);

    frameClumpInfo.dimensions = vec4(1,1,1,0);
    frameClumpInfo.dimensions.x = newVoxInfo.allModelsInFile[0].xSize;
    frameClumpInfo.dimensions.y = newVoxInfo.allModelsInFile[0].ySize;
    frameClumpInfo.dimensions.z = newVoxInfo.allModelsInFile[0].zSize;
    vec4 translate = vec4(vec3(0,0,0),1);
    mat4 frameTransformMat = mat4(vec4(initialClumpNormals[0], 0), vec4(initialClumpNormals[1],0), vec4(initialClumpNormals[2],0), translate);

    frameClumpInfo.transformationMatrix = frameTransformMat;

    struct clumpInfo frameClumpInfoArr[1];
    frameClumpInfoArr[0] = frameClumpInfo;

    GLuint clumpArrSSBO;
    glGenBuffers(1, &clumpArrSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clumpArrSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(frameClumpInfoArr), frameClumpInfoArr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, clumpArrSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Loading up vox info
    int voxInfoArr[newVoxInfo.allModelsInFile[0].voxelArray.size()];
    std::copy(newVoxInfo.allModelsInFile[0].voxelArray.begin(), newVoxInfo.allModelsInFile[0].voxelArray.end(), voxInfoArr);

    GLuint voxInfoSSBO;
    glGenBuffers(1, &voxInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxInfoSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxInfoArr), voxInfoArr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, voxInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


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


        cameraInfo frameCamInfo;
        frameCamInfo.cameraPos = vec4(CameraPos,0);
        frameCamInfo.cameraDir = vec4(CameraDir,0);
        frameCamInfo.cameraRight = vec4(CameraRight,0);
        frameCamInfo.cameraUp = vec4(CameraUp,0);
        struct cameraInfo frameCamInfoArr[1];
        frameCamInfoArr[0] = frameCamInfo;

        //When migrating to rust, separate this out to  initialization and use getters to get the needed info
        //when it updates


//        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
//            test(frameClumpInfo, frameCamInfo, glm::vec2(1920/2,1080/2));
//        }

        GLuint frameCameraSSBO;
        glGenBuffers(1, &frameCameraSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, frameCameraSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(frameCamInfoArr), frameCamInfoArr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, frameCameraSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);



       // glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glClear(GL_COLOR_BUFFER_BIT);

        //Draw the triangle
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Filled mode
        glDrawElements(GL_TRIANGLES, sizeof(b1boxIndices), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //Frame rate limiter with 300 fps target
//        while (glfwGetTime() < frameLimitTime + 1.0/targetFrameRate) {
//           //Do nothing here, just waiting till loop ends to limit framerate
//        }
        frameLimitTime += 1.0/targetFrameRate;


    }
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}


