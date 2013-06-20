float PI = 3.14159265358979323846264;

void
main()
{
  gl_Position = gl_Vertex;
  gl_TexCoord[0] = vec4(gl_Vertex.x/2-0.25, gl_Vertex.y/2-0.5, 1.0f, 1.0f);
}