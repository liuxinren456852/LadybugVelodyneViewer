#include "LadybugVelodyne.h"

#include <QtWidgets>

#include <sstream>

LadybugVelodyne::LadybugVelodyne(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
	, timer (new QTimer(this))
{
	ui.setupUi(this);
	connect(timer, SIGNAL(timeout()), ui.viewer, SLOT(advance()));
}

LadybugVelodyne::~LadybugVelodyne()
{

}

void LadybugVelodyne::on_actionOpenLadybug_triggered()
{
	QString filename = /*"Z:\\skynet\\ladybug-capture-179000.pgr";//*/ QFileDialog::getOpenFileName(this, tr("Open Ladybug File"), QString("c:\\tmp"), tr("Point Grey Ladybug Stream (*.pgr)"));

	if(!filename.isEmpty())
	{
		ui.viewer->openLadybug(filename);
	}
}

void LadybugVelodyne::on_actionOpenVelodyne_triggered()
{
	QString filename = /*"Z:\\skynet\\udp_log 2013-07-03 11 48 16PM-velo.dat";//*/ QFileDialog::getOpenFileName(this, tr("Open Skynet UDP Log File"), QString("c:\\tmp"), tr("Skynet UDP Log (*.dat)"));

	if(!filename.isEmpty())
	{
		ui.viewer->openVelodyne(filename);
	}
}

void LadybugVelodyne::on_play_clicked()
{
	start_play(!timer->isActive());
}

void LadybugVelodyne::start_play(bool start)
{
	if (start)
		timer->start(16);
	else
		timer->stop();
}

void LadybugVelodyne::on_viewer_timeUpdate(int time)
{
	std::stringstream ss;
	ss << time;
	ui.statusBar->showMessage(QString(ss.str().c_str()));
}