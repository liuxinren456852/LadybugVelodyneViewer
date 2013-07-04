#version 120

uniform sampler2D tex;

varying vec4 texCoord;

void main(void)
{
  gl_FragColor = vec4(texture2D(tex, texCoord.st).rgb, 1.0f);
}