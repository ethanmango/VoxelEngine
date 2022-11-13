#version 330 core
layout (location = 0) in vec3 aPos; //This shader gets called on each individual vertex, so we input only the vertex

//uniform stays the same for any vertex passed in
uniform mat4 MVP;

void main(){
    gl_Position =  vec4(aPos.x, aPos.y, aPos.z, 1.0); //gl_position is an predefined output
}

