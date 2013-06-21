void
main()
{
  gl_Position = gl_Vertex;
  gl_TexCoord[0] = vec4(gl_Vertex.x/2.0f-0.25f, gl_Vertex.y/2.0f-0.5f, 1.0f, 1.0f);
}