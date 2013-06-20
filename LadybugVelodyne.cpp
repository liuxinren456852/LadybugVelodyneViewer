#include "LadybugVelodyne.h"

#include "ViewerWidget.h"

#include <QtGui>

LadybugVelodyne::LadybugVelodyne(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	QWidget * window = new QWidget(this);

	QLayout * layout = new QVBoxLayout(window);
	window->setLayout(layout);

	QSlider * slider = new QSlider(Qt::Horizontal, window);
	slider->setMinimumSize(2048, 40);
	slider->setMaximumSize(2048, 40);
	layout->addWidget(slider);

	QPushButton * button = new QPushButton("Play/Pause", window);
	button->setMinimumSize(2048, 40);
	button->setMaximumSize(2048, 40);
	layout->addWidget(button);

	ViewerWidget * viewer = new ViewerWidget(window);
	viewer->setMinimumSize(2048, 1024);
	viewer->setMaximumSize(2048, 1024);
	layout->addWidget(viewer);

	this->setCentralWidget(window);
	
	slider->setMinimum(45000);
	slider->setMaximum(77000);

	connect(button, SIGNAL(clicked()), viewer, SLOT(togglePlay()));
	connect(slider, SIGNAL(valueChanged(int)), viewer, SLOT(setTime(int)));
	viewer->setTime(slider->minimum());
}

LadybugVelodyne::~LadybugVelodyne()
{

}
