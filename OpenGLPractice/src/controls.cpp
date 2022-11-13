#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.h"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

// Initial position : on +Z
glm::vec3 position = glm::vec3( 0, 0, 10 );
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 3.0f; // 3 units / second
float mouseSpeed = 0.005f;


void computeMatricesFromInputs(GLFWwindow* window, int width, int height){
    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    //TODO: Stuttering issue when moving keyboard and mouse at same time, but will figure out within rust.
    //Most likely has to do with when this code is being called in the loop. Not important for now.

    //Position dictated by keyboard, direction dictated by mouse
    //In an FPS, the camera position is dictated by player position (keyboard)
    //The camera direction is dictated by the mouse look direction

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, width/2, height/2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float(width/2 - xpos );
    verticalAngle   += mouseSpeed * float( height/2 - ypos );
    if (verticalAngle > .5f) verticalAngle = .5f;
    if (verticalAngle < -.5f) verticalAngle = -.5;

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
            cos(verticalAngle) * sin(horizontalAngle),
            sin(verticalAngle),
            cos(verticalAngle) * cos(horizontalAngle)
    );

    // Right vector
    glm::vec3 right = glm::vec3(
            sin(horizontalAngle - 3.14f/2.0f),
            0,
            cos(horizontalAngle - 3.14f/2.0f)
    );

    // Up vector
    glm::vec3 up = glm::cross( right, direction );

    // Move forward
    if (glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
        position -= right * deltaTime * speed;
    }

    float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

    // Camera matrix
    ViewMatrix       = glm::lookAt(
            position,           // Camera is here
            position+direction, // and looks here : at the same position, plus "direction"
            up                  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}
glm::mat4 getViewMatrix(){
    return ViewMatrix;
};
glm::mat4 getProjectionMatrix(){
    return ProjectionMatrix;
};

