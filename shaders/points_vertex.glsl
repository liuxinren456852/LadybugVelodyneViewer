#version 120

attribute vec3 vertex;

uniform float age;
uniform vec3 center;
uniform float ladybug_rotation;
uniform float velodyne_rotation_y;
uniform float velodyne_rotation_z;
uniform float opacity;

varying vec4 color;

float PI = 3.14159265358979323846264f;

void main(void)
{
	float cos_velodyne_rotation_z = cos(velodyne_rotation_z);
	float sin_velodyne_rotation_z = sin(velodyne_rotation_z);
	float cos_velodyne_rotation_y = cos(velodyne_rotation_y);
	float sin_velodyne_rotation_y = sin(velodyne_rotation_y);
	float cos_ladybug_rotation = cos(ladybug_rotation);
	float sin_ladybug_rotation = sin(ladybug_rotation);
	vec3 velodyne_rotated_z = vec3(cos_velodyne_rotation_z*vertex.x - sin_velodyne_rotation_z*vertex.y, sin_velodyne_rotation_z*vertex.x + cos_velodyne_rotation_z*vertex.y, vertex.z);
	vec3 velodyne_rotated_y = vec3(cos_velodyne_rotation_y*velodyne_rotated_z.x - sin_velodyne_rotation_y*velodyne_rotated_z.z, velodyne_rotated_z.y, sin_velodyne_rotation_y*velodyne_rotated_z.x + cos_velodyne_rotation_y*velodyne_rotated_z.z);
	vec3 ladybug_rotated = vec3(cos_ladybug_rotation*velodyne_rotated_y.x - sin_ladybug_rotation*velodyne_rotated_y.y, sin_ladybug_rotation*velodyne_rotated_y.x + cos_ladybug_rotation*velodyne_rotated_y.y, velodyne_rotated_y.z);
	vec3 offset = ladybug_rotated - center;
	float r = sqrt(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);
	gl_Position = vec4(-atan(offset.y, offset.x)/PI, -(2.0f*acos(offset.z/r)/PI-1.0f), 1.0f, 1.0f);
	float val = 1.0f-(r-0.5f)/25.0f;

	color = vec4(1.0f, 1.0f, 1.0f, opacity*(1.0f-age));
	
	if (val < 0.25f)
	{
		color.r = 0.0f;
		color.g = 4.0f*val;
	}
	else if (val < 0.5f)
	{
		color.r = 0.0f;
		color.b = 1.0f + 4.0f*(0.25f - val);
	}
	else if (val < 0.75f)
	{
		color.r = 4.0f*(val - 0.5f);
		color.b = 0.0f;
	}
	else
	{
		color.g = 1.0f + 4.0f*(0.75f - val);
		color.b = 0.0f;
	}
	gl_PointSize = 2.0f;

	if (val > 1.0f || val < 0.0f)
		gl_Position.z = 2.0f;
}