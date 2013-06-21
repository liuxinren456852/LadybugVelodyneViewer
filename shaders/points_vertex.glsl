float PI = 3.14159265358979323846264f;
float velodyne_rotation_z = 0.00610865238f;
float velodyne_rotation_y = -0.0436332313f;
float ladybug_rotation = 0.0610865238f;//0.0523598776;

void
main()
{
	vec3 center = vec3(-0.5715f, 0.1143f, 0.2921f);
	vec3 velodyne_rotated_z = vec3(cos(velodyne_rotation_z)*gl_Vertex.x - sin(velodyne_rotation_z)*gl_Vertex.y, sin(velodyne_rotation_z)*gl_Vertex.x + cos(velodyne_rotation_z)*gl_Vertex.y, gl_Vertex.z);
	vec3 velodyne_rotated_y = vec3(cos(velodyne_rotation_y)*velodyne_rotated_z.x - sin(velodyne_rotation_y)*velodyne_rotated_z.z, velodyne_rotated_z.y, sin(velodyne_rotation_y)*velodyne_rotated_z.x + cos(velodyne_rotation_y)*velodyne_rotated_z.z);
	vec3 ladybug_rotated = vec3(cos(ladybug_rotation)*velodyne_rotated_y.x - sin(ladybug_rotation)*velodyne_rotated_y.y, sin(ladybug_rotation)*velodyne_rotated_y.x + cos(ladybug_rotation)*velodyne_rotated_y.y, velodyne_rotated_y.z);
	vec3 offset = ladybug_rotated - center;
	float r = sqrt(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);
	gl_Position = vec4(-atan(offset.y, offset.x)/PI, -(2.0f*acos(offset.z/r)/PI-1.0f), 1.0f, 1.0f);
	float val = 1.0f-(r-2.0f)/25.0f;

	gl_FrontColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (val < 0.25f)
	{
		gl_FrontColor.r = 0.0f;
		gl_FrontColor.g = 4.0f*val;
	}
	else if (val < 0.5f)
	{
		gl_FrontColor.r = 0.0f;
		gl_FrontColor.b = 1.0f + 4.0f*(0.25f - val);
	}
	else if (val < 0.75f)
	{
		gl_FrontColor.r = 4.0f*(val - 0.5f);
		gl_FrontColor.b = 0.0f;
	}
	else
	{
		gl_FrontColor.g = 1.0f + 4.0f*(0.75f - val);
		gl_FrontColor.b = 0.0f;
	}

	if (val > 1.0f)
		gl_FrontColor.a = 0.0f;
}