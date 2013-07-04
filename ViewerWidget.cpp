#include "ViewerWidget.h"

#include <sstream>
#include <cassert>
#include <iostream>
#include <iomanip>

#include <stdint.h>

#include <ladybugrenderer.h>

double ViewerWidget::velodyne_range_scale = 0.002;
double ViewerWidget::velodyne_bottom_to_top_dist = 0.0762;

ViewerWidget::ViewerWidget(QWidget * parent)
	: QGLWidget(parent)
	, panorama_shader(0)
	, points_shader(0)
	, ladybug_context(0)
	, ladybug_stream_context(0)
	, num_ladybug_images(0)
	, ladybug_loader(0)
	, velodyne_loader(0)
	, ladybug_fbo(0)
	, ladybug_quad_vertices(0)
	, ladybug_idx(0)
	, velodyne_idx(0)
	, nLadybugTex(20)
	, nVelodyneBuf(20)
	, velodyne_rotation_z(0.075921819f)
	, velodyne_rotation_y(-0.017453298f)
	, ladybug_rotation(0.47996575f)//0.0523598776;
	, center(QVector3D(-0.5715f, 0.1143f,0.42209989f))	
{
	setMinimumHeight(512);
	setMinimumWidth(1024);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	ladybug_tex.resize(nLadybugTex);
	ladybug_tex_lru.resize(nLadybugTex);
	ladybug_tex_map_inv.resize(nLadybugTex);

	velodyne_buf.resize(nVelodyneBuf);
	velodyne_buf_lru.resize(nVelodyneBuf);
	velodyne_buf_map_inv.resize(nVelodyneBuf);

	loadVelodyneCalibrations();
}

ViewerWidget::~ViewerWidget()
{
	closeVelodyne();
	closeLadybug();

	unloadShaders();
}

void ViewerWidget::checkLadybug(const LadybugError & error) const
{
	if (LADYBUG_OK != error)
	{
		std::cerr << ladybugErrorToString(error) << std::endl;
		assert(0);
	}
}

unsigned short ViewerWidget::ntohs(unsigned short in) const
{
	unsigned char a = in >> 8;
	unsigned char b = in;
	return (b << 8)|a;
}

unsigned long ViewerWidget::ntohl(unsigned long in) const
{
	unsigned char a = in >> 24;
	unsigned char b = in >> 16;
	unsigned char c = in >> 8;
	unsigned char d = in;

	return (d<<24)|(c<<16)|(b<<8)|a;
}

void ViewerWidget::unloadShaders()
{
	if (panorama_shader)
		delete panorama_shader;

	if (points_shader)
		delete points_shader;

	panorama_shader = 0;
	points_shader = 0;
}

void ViewerWidget::loadShaders()
{
	panorama_shader = new QOpenGLShaderProgram();
	panorama_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/panorama_vertex.glsl");
	panorama_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/panorama_fragment.glsl");
	panorama_shader->link();

	points_shader = new QOpenGLShaderProgram();
	points_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/points_vertex.glsl");
	points_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/points_fragment.glsl");
	points_shader->link();
}

void ViewerWidget::loadVelodyneCalibrations()
{
	std::ifstream velodyneCalib ("C:\\Users\\Kevin James Matzen\\Desktop\\velodyne_calibration.csv");

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

GLuint ViewerWidget::ladybugLRU(unsigned int idx)
{
	for (std::vector<int>::iterator i = ladybug_tex_lru.begin(); i != ladybug_tex_lru.end(); ++i)
		++(*i);
	std::map<unsigned int,size_t>::iterator i = ladybug_tex_map.find(idx);
	if (ladybug_tex_map.end() != i)
	{
		ladybug_tex_lru[i->second] = 0;
		return ladybug_tex[i->second];
	}

	std::vector<int>::iterator m = std::max_element(ladybug_tex_lru.begin(), ladybug_tex_lru.end());
	size_t pos = m - ladybug_tex_lru.begin();
	const unsigned int & t_old = ladybug_tex_map_inv[pos];
	ladybug_tex_map.erase(t_old);
	ladybug_tex_map[idx] = pos;
	ladybug_tex_map_inv[pos] = idx;
	ladybug_tex_lru[pos] = 0;
	generatePanorama(ladybug_tex[pos], idx);
	return ladybug_tex[pos];
}

QOpenGLBuffer * ViewerWidget::velodyneLRU(unsigned int idx)
{
	for (std::vector<int>::iterator i = velodyne_buf_lru.begin(); i != velodyne_buf_lru.end(); ++i)
		++(*i);
	std::map<unsigned int,size_t>::iterator it = velodyne_buf_map.find(idx);
	if (velodyne_buf_map.end() != it)
	{
		velodyne_buf_lru[it->second] = 0;
		return velodyne_buf[it->second];
	}

	std::vector<int>::iterator m = std::max_element(velodyne_buf_lru.begin(), velodyne_buf_lru.end());
	size_t pos = m - velodyne_buf_lru.begin();
	const unsigned int & t_old = velodyne_buf_map_inv[pos];
	velodyne_buf_map.erase(t_old);
	velodyne_buf_map[idx] = pos;
	velodyne_buf_map_inv[pos] = idx;
	velodyne_buf_lru[pos] = 0;
	generatePoints(velodyne_buf[pos], idx);
	return velodyne_buf[pos];
}

void ViewerWidget::generatePanorama(GLuint tex, unsigned int idx)
{
	QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

	assert(ladybug_keys.size());

	LadybugImage image;
	checkLadybug(ladybugGoToImage(ladybug_stream_context, ladybug_keys[idx].second));
	checkLadybug(ladybugReadImageFromStream(ladybug_stream_context, &image));
	checkLadybug(ladybugConvertImage(ladybug_context, &image, NULL));
	checkLadybug(ladybugUpdateTextures(ladybug_context, LADYBUG_NUM_CAMERAS, NULL));
	checkLadybug(ladybugRenderOffScreenImage(ladybug_context, LADYBUG_PANORAMIC, LADYBUG_BGR32F, NULL)); 

	unsigned int uiHeight, uiWidth;
	checkLadybug(ladybugGetOffScreenImageSize(ladybug_context, LADYBUG_PANORAMIC, &uiWidth, &uiHeight));

	glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, ladybug_fbo);
	glBindTexture(GL_TEXTURE_2D, tex);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, uiWidth, uiHeight, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ViewerWidget::drawPanorama()
{
	QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

	panorama_shader->bind();
	ladybug_quad_vertices->bind();
	panorama_shader->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
	panorama_shader->setUniformValue("tex", 0);
	panorama_shader->enableAttributeArray("vertex");

	GLuint tex = ladybugLRU(ladybug_idx);

	glFuncs.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glDrawArrays(GL_QUADS, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void ViewerWidget::generatePoints(QOpenGLBuffer * buf, unsigned int idx)
{
	assert(velodyne_keys.size());

	velodyne_file.seekg(velodyne_keys[idx].second);

	buf->bind();
	char * ptr = (char*)buf->map(QOpenGLBuffer::WriteOnly);
	for (int i = 0; i < 12; ++i)
	{
		uint16_t header;
		uint16_t rotation;

		velodyne_file.read(reinterpret_cast<char*>(&header), sizeof(header));
		velodyne_file.read(reinterpret_cast<char*>(&rotation), sizeof(rotation));

		assert (header == 0xEEFF || header == 0xDDFF);

		int block_offset = (header == 0xEEFF) ? 0 : 32;

		assert (rotation < 36000);

		float rot_sin = sin((rotation/100.0f)/180.0f*M_PI);
		float rot_cos = cos((rotation/100.0f)/180.0f*M_PI);

		for (int j = 0; j < 32; ++j)
		{
			int laser_num = block_offset + j;

			uint16_t laser;
			uint8_t intensity;

			velodyne_file.read(reinterpret_cast<char*>(&laser), sizeof(laser));

			velodyne_file.read(reinterpret_cast<char*>(&intensity), sizeof(intensity));

			assert(velodyne_file);

			float px = 0, py = 0, pz = 0;
			if (laser != 0)
			{
				const VelodyneCalibration & calibration = velodyneCalibrations[laser_num];

				float laser_range = laser*velodyne_range_scale + calibration.dcf;
					
				float cos_rot_angle = rot_cos*calibration.cosrcf + rot_sin*calibration.sinrcf;
				float sin_rot_angle = rot_sin*calibration.cosrcf - rot_cos*calibration.sinrcf;

				float voff = (block_offset == 0) ? calibration.voff : calibration.voff + velodyne_bottom_to_top_dist;

				float xy_range = laser_range*calibration.cosvcf - voff*calibration.sinvcf;

				px = xy_range*sin_rot_angle - calibration.hoff*cos_rot_angle;
				py = xy_range*cos_rot_angle + calibration.hoff*sin_rot_angle;
				pz = laser_range*calibration.sinvcf + voff*calibration.cosvcf;

				float t = px;
				px = py;
				py = -t;
			}

			memcpy(ptr, &px, sizeof(px));
			ptr += sizeof(px);
			memcpy(ptr, &py, sizeof(py));
			ptr += sizeof(py);
			memcpy(ptr, &pz, sizeof(pz));
			ptr += sizeof(pz);
		}
	}
	buf->unmap();
}

void ViewerWidget::drawPoints()
{
	unsigned int start = velodyne_idx;
	while (time - velodyne_keys[start].first < 1000 && start > 0)
		--start;

	points_shader->bind();
	glEnable(GL_POINT_SMOOTH);
	glPointSize(2.0f);
	points_shader->enableAttributeArray("vertex");

	points_shader->setUniformValue("velodyne_rotation_y", velodyne_rotation_y);
	points_shader->setUniformValue("velodyne_rotation_z", velodyne_rotation_z);
	points_shader->setUniformValue("ladybug_rotation", ladybug_rotation);
	points_shader->setUniformValue("center", center);

	for (unsigned int i = start; i <= velodyne_idx; ++i)
	{
		uint64_t t = velodyne_keys[i].first;

		QOpenGLBuffer * buf = velodyneLRU(i);

		buf->bind();
		points_shader->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
		points_shader->setUniformValue("age", (time-t)/1000.0f);

		glClear(GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_POINTS, 0, 12*32);
	}
}

void ViewerWidget::setTime(int value)
{
	time = value;
	if (ladybug_keys.size())
	{
		while (time > ladybug_keys[ladybug_idx].first && ladybug_idx < ladybug_keys.size() - 1)
			++ladybug_idx;
		while (time < ladybug_keys[ladybug_idx].first && ladybug_idx > 0)
			--ladybug_idx;
	}
	if (velodyne_keys.size())
	{
		while (time > velodyne_keys[velodyne_idx].first && velodyne_idx < velodyne_keys.size() - 1)
			++velodyne_idx;
		while (time < velodyne_keys[velodyne_idx].first && velodyne_idx > 0)
			--velodyne_idx;
	}
	update();
}

void ViewerWidget::advance()
{
	setTime(time+166);
	emit timeUpdate(time);
}

void ViewerWidget::retreat()
{
	setTime(time-166);
	emit timeUpdate(time);
}

void ViewerWidget::initializeGL()
{
	loadShaders();
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

void ViewerWidget::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
}

void ViewerWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);

	if (2 == both_loaded.load())
	{
		drawPanorama();

		glClear(GL_DEPTH_BUFFER_BIT);

		drawPoints();
	}

	std::cout << glGetError() << std::endl;
}

void ViewerWidget::velodyneTiltUp()
{
	velodyne_rotation_y += 0.00872664626f;
	update();
}

void ViewerWidget::velodyneTiltDown()
{
	velodyne_rotation_y -= 0.00872664626f;
	update();
}

void ViewerWidget::velodyneRollLeft()
{
	velodyne_rotation_z += 0.00872664626f;
	update();
}

void ViewerWidget::velodyneRollRight()
{
	velodyne_rotation_z -= 0.00872664626f;
	update();
}

void ViewerWidget::ladybugRotateLeft()
{
	ladybug_rotation += 0.00872664626f;
	update();
}

void ViewerWidget::ladybugRotateRight()
{
	ladybug_rotation -= 0.00872664626f;
	update();
}

void ViewerWidget::xminus()
{
	center.setX(center.x() + 0.01);
	update();
}

void ViewerWidget::xplus()
{
	center.setX(center.x() - 0.01);
	update();
}

void ViewerWidget::yminus()
{
	center.setY(center.y() - 0.01);
	update();
}

void ViewerWidget::yplus()
{
	center.setY(center.y() + 0.01);
	update();
}

void ViewerWidget::zminus()
{
	center.setZ(center.z() + 0.01);
	update();
}

void ViewerWidget::zplus()
{
	center.setZ(center.z() - 0.01);
	update();
}


void ViewerWidget::openVelodyne(const QString & s)
{
	if (1 == velodyne_loaded.load())
	{
		velodyne_loaded.deref();
		both_loaded.deref();
		emit playerState(false);
	}

	closeVelodyne();

	velodyne_filename = s;
	velodyne_file.open(velodyne_filename.toStdString().c_str(), std::ios_base::binary);

	for (std::vector<QOpenGLBuffer*>::iterator i = velodyne_buf.begin(); i != velodyne_buf.end(); ++i)
	{
		*i = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		(*i)->setUsagePattern(QOpenGLBuffer::DynamicDraw);
		(*i)->create();
		(*i)->bind();
		(*i)->allocate(sizeof(float)*3*32*12);
	}

	velodyne_loader = new VelodyneLoader(this);
	velodyne_loader->start();
}

void ViewerWidget::openLadybug(const QString & s)
{
	QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

	if (1 == ladybug_loaded.load())
	{
		ladybug_loaded.deref();
		both_loaded.deref();
		emit playerState(false);
	}

	closeLadybug();

	checkLadybug(ladybugCreateStreamContext(&ladybug_stream_context));

	ladybug_filename = s;
	checkLadybug(ladybugInitializeStreamForReading(ladybug_stream_context, ladybug_filename.toStdString().c_str(), true));

	checkLadybug(ladybugGetStreamNumOfImages(ladybug_stream_context, &num_ladybug_images));
	emit numLadybugFramesChanged(num_ladybug_images);

	checkLadybug(ladybugCreateContext(&ladybug_context));

	QString ladybug_config_filename = ladybug_filename + ".config";
	checkLadybug(ladybugGetStreamConfigFile(ladybug_stream_context, ladybug_config_filename.toStdString().c_str()));
	checkLadybug(ladybugLoadConfig(ladybug_context, ladybug_config_filename.toStdString().c_str()));
	checkLadybug(ladybugConfigureOutputImages(ladybug_context, LADYBUG_PANORAMIC));
	checkLadybug(ladybugSetPanoramicViewingAngle(ladybug_context, LADYBUG_FRONT_1_POLE_5));
	//checkLadybug(ladybugSetColorProcessingMethod(ladybug_context, LADYBUG_HQLINEAR_GPU));

	if (velodyne_loader)
	{
		velodyne_buf_map.clear();
		for (std::vector<QOpenGLBuffer*>::iterator i = velodyne_buf.begin(); i != velodyne_buf.end(); ++i)
			delete *i;
	}

	unloadShaders();

	checkLadybug(ladybugSetDisplayWindow(ladybug_context));

	if (velodyne_loader)
	{
		for (std::vector<QOpenGLBuffer*>::iterator i = velodyne_buf.begin(); i != velodyne_buf.end(); ++i)
		{
			*i = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
			(*i)->setUsagePattern(QOpenGLBuffer::DynamicDraw);
			(*i)->create();
			(*i)->bind();
			(*i)->allocate(sizeof(float)*3*32*12);
		}
	}

	LadybugImage image;
	checkLadybug(ladybugReadImageFromStream(ladybug_stream_context, &image));
	checkLadybug(ladybugConvertImage(ladybug_context, &image, NULL));
	checkLadybug(ladybugUpdateTextures(ladybug_context, LADYBUG_NUM_CAMERAS, NULL));
	checkLadybug(ladybugRenderOffScreenImage(ladybug_context, LADYBUG_PANORAMIC, LADYBUG_BGR32F, NULL)); 

	unsigned int * puiID;
	float fROIWidth, fROIHeight;
	checkLadybug(ladybugGetOpenGLTextureID(ladybug_context, LADYBUG_PANORAMIC, &puiID, &fROIWidth, &fROIHeight));
	
	unsigned int uiHeight, uiWidth;
	checkLadybug(ladybugGetOffScreenImageSize(ladybug_context, LADYBUG_PANORAMIC, &uiWidth, &uiHeight));

	glGenTextures((GLsizei)ladybug_tex.size(), &ladybug_tex[0]);
	for (std::vector<GLuint>::iterator i = ladybug_tex.begin(); i != ladybug_tex.end(); ++i)
	{
		glBindTexture(GL_TEXTURE_2D, *i);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, uiWidth, uiHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, NULL);
	}	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glFuncs.glGenFramebuffers(1, &ladybug_fbo);
	glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, ladybug_fbo);
	glFuncs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *puiID, 0);
	glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ladybug_quad_vertices = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	ladybug_quad_vertices->setUsagePattern(QOpenGLBuffer::StaticDraw);
	ladybug_quad_vertices->create();
	std::vector<QVector3D> vertices;
		
	vertices.push_back(QVector3D(-1, -1, 1));
	vertices.push_back(QVector3D(-1, 1, 1));
	vertices.push_back(QVector3D(1, 1, 1));
	vertices.push_back(QVector3D(1, -1, 1));

	ladybug_quad_vertices->bind();
	ladybug_quad_vertices->allocate(&vertices[0], (int)vertices.size()*sizeof(QVector3D));

	loadShaders();
	
	ladybug_loader = new LadybugLoader(this);
	ladybug_loader->start();
}

void ViewerWidget::closeVelodyne()
{
	if (velodyne_loader)
	{
		velodyne_loader->stop();
		while (!velodyne_loader->isFinished()) QThread::msleep(10);
		delete velodyne_loader;
		velodyne_loader = 0;

		velodyne_file.close();
		velodyne_keys.clear();

		velodyne_buf_map.clear();
		for (std::vector<QOpenGLBuffer*>::iterator i = velodyne_buf.begin(); i != velodyne_buf.end(); ++i)
			delete *i;
	}

}

void ViewerWidget::closeLadybug()
{
	if (ladybug_loader)
	{
		QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

		ladybug_loader->stop();
		while(!ladybug_loader->isFinished()) QThread::msleep(10);
		delete ladybug_loader;
		ladybug_loader = 0;

		delete ladybug_quad_vertices;

		ladybug_tex_map.clear();

		checkLadybug(ladybugReleaseOffScreenImage(ladybug_context, LADYBUG_PANORAMIC));

		glFuncs.glDeleteFramebuffers(1, &ladybug_fbo);
		
		glDeleteTextures((GLsizei)ladybug_tex.size(), &ladybug_tex[0]);
		ladybug_tex_map.clear();

		checkLadybug(ladybugDestroyContext(&ladybug_context));

		checkLadybug(ladybugStopStream(ladybug_stream_context));
		checkLadybug(ladybugDestroyStreamContext(&ladybug_stream_context));
		ladybug_keys.clear();
	}

}

void ViewerWidget::activate()
{
	emit playerState(true);
	uint64_t start = std::numeric_limits<uint64_t>::max();
	uint64_t end = std::numeric_limits<uint64_t>::min();
	if (velodyne_keys.size())
	{
		start = std::min(velodyne_keys.front().first, start);
		end = std::max(velodyne_keys.back().first, end);
	}
	if (ladybug_keys.size())
	{
		start = std::min(ladybug_keys.front().first, start);
		end = std::max(ladybug_keys.back().first, end);
	}
	emit timeRange(start, end);

	setTime(start);
	emit timeUpdate(start);
}

void ViewerWidget::LadybugLoader::run()
{
	QString cached = viewer_->ladybug_filename + ".cache";
	std::ifstream ladybug_keys_cached (cached.toStdString().c_str(), std::ios_base::binary);
	if (ladybug_keys_cached)
	{
		for (unsigned int i = 0; i < viewer_->num_ladybug_images; ++i)
		{
			if (0 == state_.load())
				break;

			std::pair<uint64_t,unsigned int> p;
			ladybug_keys_cached.read(reinterpret_cast<char*>(&p.first), sizeof(p.first));
			ladybug_keys_cached.read(reinterpret_cast<char*>(&p.second), sizeof(p.second));
			assert (ladybug_keys_cached);
			viewer_->ladybug_keys.push_back(p);

			emit viewer_->numLadybugFramesLoaded(i+1);
		}
		ladybug_keys_cached.close();
	}
	else
	{
		std::stringstream tmp_cache;

		LadybugImage image;
					
		for (unsigned int i = 0; i < viewer_->num_ladybug_images; ++i)
		{
			if (0 == state_.load())
				break;

			viewer_->checkLadybug(ladybugGoToImage(viewer_->ladybug_stream_context, i));
			viewer_->checkLadybug(ladybugReadImageFromStream(viewer_->ladybug_stream_context, &image));
			unsigned char * buf = image.pData + (image.uiDataSizeBytes - 11);
			uint16_t seconds = *reinterpret_cast<uint16_t*>(buf);
			buf += sizeof(seconds);
			uint32_t ticks = *reinterpret_cast<uint32_t*>(buf);
			uint64_t time = 10000*seconds + ticks;
			std::pair<uint64_t,unsigned int> p (time, i);
			viewer_->ladybug_keys.push_back(p);

			tmp_cache.write(reinterpret_cast<char*>(&p.first), sizeof(p.first));
			tmp_cache.write(reinterpret_cast<char*>(&p.second), sizeof(p.second));

			emit viewer_->numLadybugFramesLoaded(i+1);
		}

		if (1 == state_.load())
		{
			std::ofstream out_cache (cached.toStdString().c_str(), std::ios_base::binary);
			std::copy(	std::istreambuf_iterator<char>(tmp_cache),
						std::istreambuf_iterator<char>(),
						std::ostreambuf_iterator<char>(out_cache));
			out_cache.close();
		}
	}

	if (1 == state_.load())
	{
		viewer_->ladybug_loaded.ref();
		viewer_->both_loaded.ref();
		if (2 == viewer_->both_loaded.load())
			viewer_->activate();
	}
}

void ViewerWidget::VelodyneLoader::run()
{
	QString cached = viewer_->velodyne_filename + ".cache";
	std::ifstream velodyne_keys_cached (cached.toStdString().c_str(), std::ios_base::binary);
	if (velodyne_keys_cached)
	{
		velodyne_keys_cached.seekg(0, std::ios_base::end);
		std::ifstream::pos_type length = velodyne_keys_cached.tellg()/(sizeof(uint64_t)+sizeof(std::ifstream::pos_type));
		velodyne_keys_cached.seekg(0);

		emit viewer_->velodyneLogLength(length);

		int pct = 0;
		for (int i = 0; i < length; ++i)
		{
			if (0 == state_.load())
				break;

			std::pair<uint64_t,std::ifstream::pos_type> p;
			velodyne_keys_cached.read(reinterpret_cast<char*>(&p.first), sizeof(p.first));
			velodyne_keys_cached.read(reinterpret_cast<char*>(&p.second), sizeof(p.second));
			if (velodyne_keys_cached)
				viewer_->velodyne_keys.push_back(p);

			int next_pct = 100*(i+1)/length;
			if (next_pct > pct)
				emit viewer_->velodyneLogPosition(i+1);
			pct = next_pct;
		}

		velodyne_keys_cached.close();
	}
	else
	{
		std::stringstream tmp_cache;
		int pct = 0;
		viewer_->velodyne_file.seekg(0, std::ios_base::end);
		std::ifstream::pos_type length = viewer_->velodyne_file.tellg();
		viewer_->velodyne_file.seekg(0);
		emit viewer_->velodyneLogLength(length);

		while (viewer_->velodyne_file)
		{
			if (0 == state_.load())
				break;
		
			uint32_t ticks, src_ip, dest_ip, len;
			uint16_t port;
		
			viewer_->velodyne_file.read(reinterpret_cast<char*>(&ticks), sizeof(ticks));
			viewer_->velodyne_file.read(reinterpret_cast<char*>(&src_ip), sizeof(src_ip));
			viewer_->velodyne_file.read(reinterpret_cast<char*>(&dest_ip), sizeof(dest_ip));
			viewer_->velodyne_file.read(reinterpret_cast<char*>(&port), sizeof(port));
			viewer_->velodyne_file.read(reinterpret_cast<char*>(&len), sizeof(len));
			if (port == 2368)
			{
				assert(len == 1212);
				std::ifstream::pos_type pos = viewer_->velodyne_file.tellg();
				viewer_->velodyne_file.seekg(len-6, std::ios_base::cur);
				uint16_t seconds;
				uint32_t ticks;
				viewer_->velodyne_file.read(reinterpret_cast<char*>(&seconds), sizeof(seconds));
				viewer_->velodyne_file.read(reinterpret_cast<char*>(&ticks), sizeof(ticks));
				uint64_t time = 10000*viewer_->ntohs(seconds) + viewer_->ntohl(ticks);

				if (viewer_->velodyne_file)
				{
					std::pair<uint64_t, std::ifstream::pos_type> p (time, pos);
					viewer_->velodyne_keys.push_back(p);
					tmp_cache.write(reinterpret_cast<char*>(&p.first), sizeof(p.first));
					tmp_cache.write(reinterpret_cast<char*>(&p.second), sizeof(p.second));
				}
			}
			else
				viewer_->velodyne_file.seekg(len, std::ios_base::cur);
			std::ifstream::pos_type pos = viewer_->velodyne_file.tellg();
			int next_pct = 100*pos/length;
			if (next_pct > pct)
				emit viewer_->velodyneLogPosition(pos);
			pct = next_pct;

		}

		viewer_->velodyne_file.clear();

		if (1 == state_.load())
		{
			std::ofstream out_cache (cached.toStdString().c_str(), std::ios_base::binary);
			std::copy(	std::istreambuf_iterator<char>(tmp_cache),
						std::istreambuf_iterator<char>(),
						std::ostreambuf_iterator<char>(out_cache));
			out_cache.close();
		}
	}

	if (1 == state_.load())
	{
		viewer_->velodyne_loaded.ref();
		viewer_->both_loaded.ref();
		if (2 == viewer_->both_loaded.load())
			viewer_->activate();
	}
}