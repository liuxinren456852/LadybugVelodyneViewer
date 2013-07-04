#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QtOpenGL>

#include <fstream>

#include <ladybug.h>
#include <ladybugstream.h>

class ViewerWidget : public QGLWidget
{
	Q_OBJECT

	struct VelodyneCalibration
	{
		double sinvcf;
		double cosvcf;
		double sinrcf;
		double cosrcf;
		double dcf;
		double voff;
		double hoff;
	};

	std::vector<VelodyneCalibration> velodyneCalibrations;

	QOpenGLShaderProgram * panorama_shader;
	QOpenGLShaderProgram * points_shader;
	
	uint64_t time;

	QAtomicInt both_loaded;

	/* LADYBUG */
	class LadybugLoader : public QThread
	{
		QAtomicInt state_;
		ViewerWidget * viewer_;
	public:
		LadybugLoader(ViewerWidget * viewer)
			: viewer_(viewer)
		{ state_.store(1); }
		void stop() { state_.store(0); }

		void run() Q_DECL_OVERRIDE;
	};

	QString ladybug_filename;
	LadybugContext ladybug_context;
	LadybugStreamContext ladybug_stream_context;
	unsigned int num_ladybug_images;
	std::vector<std::pair<uint64_t,unsigned int> > ladybug_keys;
	LadybugLoader * ladybug_loader;
	QAtomicInt ladybug_loaded;

	GLuint ladybug_fbo;
	std::vector<GLuint> ladybug_tex;
	std::map<unsigned int,size_t> ladybug_tex_map;
	std::vector<unsigned int> ladybug_tex_map_inv;
	std::vector<int> ladybug_tex_lru;

	QOpenGLBuffer * ladybug_quad_vertices;

	unsigned int ladybug_idx;
	
	const int nLadybugTex;
	/***********/

	/* Velodyne */
	class VelodyneLoader : public QThread
	{
		QAtomicInt state_;
		ViewerWidget * viewer_;
	public:
		VelodyneLoader(ViewerWidget * viewer)
			: viewer_(viewer)
		{ state_.store(1); }
		void stop() { state_.store(0); }

		void run() Q_DECL_OVERRIDE;
	};

	QString velodyne_filename;
	std::ifstream velodyne_file;
	std::vector<std::pair<uint64_t,std::ifstream::pos_type> > velodyne_keys;
	VelodyneLoader * velodyne_loader;
	QAtomicInt velodyne_loaded;

	std::vector<QOpenGLBuffer*> velodyne_buf;
	std::map<unsigned int,size_t> velodyne_buf_map;
	std::vector<unsigned int> velodyne_buf_map_inv;
	std::vector<int> velodyne_buf_lru;

	unsigned int velodyne_idx;

	const int nVelodyneBuf;
	/***********/

	float velodyne_rotation_z;
	float velodyne_rotation_y;
	float ladybug_rotation;
	QVector3D center;

	static double velodyne_range_scale;
	static double velodyne_bottom_to_top_dist;

	unsigned short ntohs(unsigned short) const;
	unsigned long ntohl(unsigned long) const;
	
	void loadShaders();
	void unloadShaders();
	
	void loadVelodyneCalibrations();
	void loadPoints();

	void generatePanorama(GLuint, unsigned int);
	void drawPanorama();
	
	void generatePoints(QOpenGLBuffer *, unsigned int);
	void drawPoints();
	
	void recordImage();
	void clean();
	void checkLadybug(const LadybugError & error) const;
	void activate();
	void setTime(int, bool);

	GLuint ladybugLRU(unsigned int idx);
	QOpenGLBuffer * velodyneLRU(unsigned int idx);

	void closeLadybug();
	void closeVelodyne();

	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;
	void resizeGL(int width, int height) Q_DECL_OVERRIDE;

signals:
	void numLadybugFramesChanged(int);
	void numLadybugFramesLoaded(int);
	void velodyneLogLength(int);
	void velodyneLogPosition(int);
	void playerState(bool);
	void timeRange(int,int);
	void timeUpdate(int);

public slots:
	void setTime(int);
	void advance();
	void retreat();

	void velodyneTiltUp();
	void velodyneTiltDown();
	void velodyneRollLeft();
	void velodyneRollRight();
	void ladybugRotateLeft();
	void ladybugRotateRight();
	void xminus();
	void xplus();
	void yminus();
	void yplus();
	void zminus();
	void zplus();

public:
	ViewerWidget(QWidget *);
	virtual ~ViewerWidget();
	void openVelodyne(const QString &);
	void openLadybug(const QString &);


};

#endif
