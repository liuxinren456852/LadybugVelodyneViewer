#version 120

attribute vec3 vertex;

varying vec4 texCoord;

void main(void)
{
  gl_Position = vec4(vertex, 1.0f);
  texCoord = vec4(vertex.x/2.0f+0.5f, 0.5f-vertex.y/2.0f, 1.0f, 1.0f);
}