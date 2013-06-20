float PI = 3.14159265358979323846264;
float velodyne_rotation_z = 0.00610865238;
float velodyne_rotation_y = -0.0436332313;
float ladybug_rotation = 0.0610865238;//0.0523598776;

void
main()
{
	vec3 center = vec3(-0.5715, 0.1143, 0.2921);
	vec3 velodyne_rotated_z = vec3(cos(velodyne_rotation_z)*gl_Vertex.x - sin(velodyne_rotation_z)*gl_Vertex.y, sin(velodyne_rotation_z)*gl_Vertex.x + cos(velodyne_rotation_z)*gl_Vertex.y, gl_Vertex.z);
	vec3 velodyne_rotated_y = vec3(cos(velodyne_rotation_y)*velodyne_rotated_z.x - sin(velodyne_rotation_y)*velodyne_rotated_z.z, velodyne_rotated_z.y, sin(velodyne_rotation_y)*velodyne_rotated_z.x + cos(velodyne_rotation_y)*velodyne_rotated_z.z);
	vec3 ladybug_rotated = vec3(cos(ladybug_rotation)*velodyne_rotated_y.x - sin(ladybug_rotation)*velodyne_rotated_y.y, sin(ladybug_rotation)*velodyne_rotated_y.x + cos(ladybug_rotation)*velodyne_rotated_y.y, velodyne_rotated_y.z);
	vec3 offset = ladybug_rotated - center;
	float r = sqrt(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);
	gl_Position = vec4(-atan(offset.y, offset.x)/PI, -(2*acos(offset.z/r)/PI-1), 1.0f, 1.0f);
	float val = 1-(r-2)/25;

	gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);

	if (val < 0.25)
	{
		gl_FrontColor.r = 0.0;
		gl_FrontColor.g = 4*val;
	}
	else if (val < 0.5)
	{
		gl_FrontColor.r = 0.0;
		gl_FrontColor.b = 1.0 + 4*(0.25 - val);
	}
	else if (val < 0.75)
	{
		gl_FrontColor.r = 4*(val - 0.5);
		gl_FrontColor.b = 0.0;
	}
	else
	{
		gl_FrontColor.g = 1.0 + 4*(0.75 - val);
		gl_FrontColor.b = 0.0;
	}

	if (val > 1.0)
		gl_FrontColor.a = 0.0;
}