#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QtOpenGL>
//#include <QtOpenGL\qgl.h>
//#include <QtOpenGL\qglshaderprogram.h>

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

	QGLShaderProgram * panoramaShader;
	QGLShaderProgram * pointsShader;
	GLuint texture;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> vertex_indices;
	
	std::multimap<double,std::streampos> velodyne_file_keys;
	std::map<std::streampos,int> velodyne_keys;
	std::vector<std::pair<int,int> > velodynes;
	std::vector<GLfloat> points;

	std::multimap<double,int> ladybug_keys;

	double time;

	int textureIndex;

	bool init;
	bool play;
	bool recording;

	std::ifstream * velodyne_file;

	static double velodyne_range_scale;
	static double velodyne_bottom_to_top_dist;

	unsigned short ntohs(unsigned short);
	unsigned long ntohl(unsigned long);
	
	void initTexture();
	void loadMesh();
	void loadShaders();
	void loadTexture();
	void loadVelodyneCalibrations();
	void loadPoints();
	void loadLadybugKeys();
	void loadVelodyneKeys();
	void drawPanorama();
	void drawPoints();
	void recordImage();
	void clean();

public slots:
	void setTime(int);
	void togglePlay();

public:
	ViewerWidget(QWidget *);
	virtual ~ViewerWidget();
	void initializeGL();
	void paintGL();
};

#endif
