#include "ViewerWidget.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <iomanip>

#define M_PI 3.14159265358979323846

double ViewerWidget::velodyne_range_scale = 0.002;
double ViewerWidget::velodyne_bottom_to_top_dist = 0.0762;

ViewerWidget::ViewerWidget(QWidget * parent)
	: QGLWidget(parent)
	, velodyne_file(new std::ifstream ("C:\\Users\\Public\\loki\\velodyne2.txt", std::ios::binary))
	, textureIndex (-1)
	, init(false)
	, play(false)
	, recording(true)
{
	time = 486.0f;
}

ViewerWidget::~ViewerWidget()
{
	velodyne_file->close();
	delete velodyne_file;
}

unsigned short ViewerWidget::ntohs(unsigned short in)
{
	unsigned char a = in >> 8;
	unsigned char b = in;
	return (b << 8)|a;
}

unsigned long ViewerWidget::ntohl(unsigned long in)
{
	unsigned char a = in >> 24;
	unsigned char b = in >> 16;
	unsigned char c = in >> 8;
	unsigned char d = in;

	return (d<<24)|(c<<16)|(b<<8)|a;
}

void ViewerWidget::loadShaders()
{
	panoramaShader = new QGLShaderProgram(this);
	panoramaShader->addShaderFromSourceFile(QGLShader::Vertex, "panorama_vertex.glsl");
	panoramaShader->addShaderFromSourceFile(QGLShader::Fragment, "panorama_fragment.glsl");
	panoramaShader->link();

	pointsShader = new QGLShaderProgram(this);
	pointsShader->addShaderFromSourceFile(QGLShader::Vertex, "points_vertex.glsl");
	pointsShader->addShaderFromSourceFile(QGLShader::Fragment, "points_fragment.glsl");
	pointsShader->link();
}

void ViewerWidget::loadMesh()
{
	vertices.push_back(-1);
	vertices.push_back(-1);
	vertices.push_back(1);

	vertices.push_back(-1);
	vertices.push_back(1);
	vertices.push_back(1);

	vertices.push_back(1);
	vertices.push_back(1);
	vertices.push_back(1);

	vertices.push_back(1);
	vertices.push_back(-1);
	vertices.push_back(1);

	vertex_indices.push_back(0);
}

void ViewerWidget::loadLadybugKeys()
{
	std::ifstream ladybug ("C:\\Users\\Public\\loki\\ladybug.csv");

	while(ladybug)
	{
		double time;
		int frame;
		char junk;

		if (!(ladybug >> time >> junk >> frame))
			break;

		assert (junk == ',');

		ladybug_keys.insert(std::make_pair(time, frame-256-188+50-7+8+1+1));//-9));
	}
}

void ViewerWidget::loadVelodyneCalibrations()
{
	std::ifstream velodyneCalib ("C:\\Users\\Public\\loki\\calibration.csv");

	while (velodyneCalib)
	{
		char junk;
		VelodyneCalibration calib;
		if (!(velodyneCalib >> calib.sinvcf >> junk >> calib.cosvcf >> junk >> calib.sinrcf >> junk >> calib.cosrcf >> junk >> calib.dcf >> junk >> calib.voff >> junk >> calib.hoff))
		{
			break;
		}
		assert (junk == ',');

		velodyneCalibrations.push_back(calib);
	}

	assert (velodyneCalibrations.size() == 64);

	velodyneCalib.close();
}

void ViewerWidget::loadVelodyneKeys()
{
	assert (*velodyne_file);

	velodyne_file->seekg(0, std::ios::end);
	std::streampos length = velodyne_file->tellg();
	velodyne_file->seekg(0, std::ios::beg);

	int nPackets = length/1216;

	assert(nPackets*1216 == length);

	for (int p = 0; p < nPackets; ++p)
	{
		unsigned int size;
		unsigned short seconds;
		unsigned int ticks;
		velodyne_file->read((char*)&size, sizeof(unsigned int));
		velodyne_file->read((char*)&seconds, sizeof(unsigned short));
		velodyne_file->read((char*)&ticks, sizeof(unsigned int));

		seconds = ntohs(seconds);
		ticks = ntohl(ticks);

		double packet_time = seconds + ticks/10000.0f;

		std::streampos pos = velodyne_file->tellg();

		velodyne_file_keys.insert(std::make_pair(packet_time, pos));

		velodyne_file->seekg(1206, std::ios::cur);

		assert (*velodyne_file);
	}
}

void ViewerWidget::clean()
{
	points.clear();
	velodyne_keys.clear();
	velodynes.clear();
}

void ViewerWidget::loadPoints()
{
	assert (*velodyne_file);

	std::multimap<double,std::streampos>::const_iterator low = velodyne_file_keys.lower_bound(time-0.1);
	std::multimap<double,std::streampos>::const_iterator high = velodyne_file_keys.lower_bound(time);

	for (std::multimap<double,std::streampos>::const_iterator p = low; p != high; ++p)
	{
		if (velodyne_keys.count(p->second))
			continue;

		velodyne_file->seekg(p->second, std::ios::beg);

		velodyne_keys[p->second] = velodynes.size();
		velodynes.push_back(std::make_pair(points.size()/3, 0));
		std::pair<int,int> & velodyne_entry = *velodynes.rbegin();

		for (int i = 0; i < 12; ++i)
		{
			unsigned short header;
			unsigned short rotation;

			velodyne_file->read((char*)&header, sizeof(unsigned short));
			velodyne_file->read((char*)&rotation, sizeof(unsigned short));

			assert (header == 0xEEFF || header == 0xDDFF);

			int block_offset = (header == 0xEEFF) ? 0 : 32;

			assert (rotation < 36000);

			double rot_sin = sin((rotation/100.0f)/180.0f*M_PI);
			double rot_cos = cos((rotation/100.0f)/180.0f*M_PI);

			for (int j = 0; j < 32; ++j)
			{
				int laser_num = block_offset + j;

				unsigned short laser;
				unsigned char intensity;

				velodyne_file->read((char*)&laser, sizeof(unsigned short));

				velodyne_file->read((char*)&intensity, sizeof(unsigned char));

				assert(*velodyne_file);

				if (laser == 0)
					continue;

				const VelodyneCalibration & calibration = velodyneCalibrations[laser_num];

				double laser_range = laser*velodyne_range_scale + calibration.dcf;
					
				double cos_rot_angle = rot_cos*calibration.cosrcf + rot_sin*calibration.sinrcf;
				double sin_rot_angle = rot_sin*calibration.cosrcf - rot_cos*calibration.sinrcf;

				double voff = (block_offset == 0) ? calibration.voff : calibration.voff + velodyne_bottom_to_top_dist;

				double xy_range = laser_range*calibration.cosvcf - voff*calibration.sinvcf;

				double px = xy_range*sin_rot_angle - calibration.hoff*cos_rot_angle;
				double py = xy_range*cos_rot_angle + calibration.hoff*sin_rot_angle;
				double pz = laser_range*calibration.sinvcf + voff*calibration.cosvcf;

				double t = px;
				px = py;
				py = -t;

				points.push_back(px);
				points.push_back(py);
				points.push_back(pz);

  				++velodyne_entry.second;
			}
		}
	}
}

void ViewerWidget::drawPanorama()
{
	panoramaShader->bind();

	glDisable(GL_DEPTH_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);

	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void ViewerWidget::drawPoints()
{
	pointsShader->bind();

	glEnable(GL_POINT_SMOOTH);
	glPointSize(2.0f);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_DEPTH_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &points[0]);

	
	std::multimap<double,std::streampos>::const_iterator low = velodyne_file_keys.lower_bound(time-0.1);
	std::multimap<double,std::streampos>::const_iterator high = velodyne_file_keys.lower_bound(time);

	for (std::multimap<double,std::streampos>::const_iterator p = low; p != high; ++p)
	{
		const std::pair<int,int> & velodyne = velodynes[velodyne_keys[p->second]];
		glDrawArrays(GL_POINTS, velodyne.first, velodyne.second);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void ViewerWidget::initTexture()
{
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void ViewerWidget::loadTexture()
{
	std::multimap<double,int>::const_iterator index = ladybug_keys.lower_bound(time);

	if (textureIndex == index->second)
		return;

	textureIndex = index->second;

	std::stringstream filename;
	filename << "C:\\Users\\Public\\loki\\ladybug_Panoramic_2048x1024_" << std::setfill('0') << std::setw(8) << index->second << ".jpg";
	QImage image (filename.str().c_str());
	if (image.isNull())
		play = false;
	if (image.depth() != 32)
		throw std::bad_alloc();
	QImage converted;
	try
	{
		converted = QGLWidget::convertToGLFormat(image);
	}
	catch (...)
	{
		throw std::bad_alloc();
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, 3, converted.width(), converted.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, converted.bits());
}

void ViewerWidget::setTime(int value)
{
	try
	{
		time = value/100.0f;
		if (init)
		{
			loadPoints();
			loadTexture();
			repaint();
		}
	}
	catch (std::bad_alloc &)
	{
		clean();
		setTime(value);
	}
}

void ViewerWidget::togglePlay()
{
	play = !play;
	update();
}

void ViewerWidget::initializeGL()
{
	init = true;
	loadVelodyneKeys();
	loadVelodyneCalibrations();
	loadShaders();
	loadMesh();
	loadLadybugKeys();
	loadPoints();
	initTexture();
	loadTexture();
}

void ViewerWidget::recordImage()
{
	QImage recording (width(), height(), QImage::Format_RGB888);
	glReadPixels(0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, recording.bits());
	QImage mirrored = recording.mirrored();
	std::stringstream ss;
	ss << "C:\\Users\\Public\\loki\\renderings\\" << time << ".png";
	mirrored.save(ss.str().c_str());
}

void ViewerWidget::paintGL()
{
	try
	{
		if (play)
		{
			time += 1/30.0f;
			loadPoints();
			loadTexture();
		}

		glViewport(0, 0, width(), height());

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		drawPanorama();

		glClear(GL_DEPTH_BUFFER_BIT);

		drawPoints();

		if (recording)
			recordImage();

		if (play)
			update();
	}
	catch(std::bad_alloc &)
	{
		clean();
		paintGL();
	}
}