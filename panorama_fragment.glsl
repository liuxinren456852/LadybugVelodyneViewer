uniform sampler2D tex;

void
main()
{
  vec4 color = texture2D(tex, gl_TexCoord[0].st);
  gl_FragColor = vec4(2*color.rgb, 1.0f);
}