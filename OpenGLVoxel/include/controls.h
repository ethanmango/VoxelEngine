#ifndef CONTROLS_HPP
#define CONTROLS_HPP

void computeMatricesFromInputs(GLFWwindow* window, int width, int height);
glm::mat4 getViewMatrix();
glm::vec3 getCameraPos();
glm::vec3 getCameraDir();
glm::vec3 getCameraRight();
glm::vec3 getCameraUp();
void setCameraPos(glm::vec3 inputPos);
void setCameraDir(glm::vec3 inputDir);

#endif
