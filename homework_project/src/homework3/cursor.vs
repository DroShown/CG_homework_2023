#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec4 cursorPos;
void main()
{
    gl_Position = vec4(aPos, 0.0)+cursorPos;
}
