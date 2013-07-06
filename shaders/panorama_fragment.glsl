#version 120

uniform sampler2D tex1;
uniform sampler2D tex2;

uniform float fade;

varying vec4 texCoord;

void main(void)
{
  vec3 a = texture2D(tex1, texCoord.st).rgb;
  vec3 b = texture2D(tex2, texCoord.st).rgb;
  gl_FragColor = vec4(mix(a, b, fade), 1.0f);
}