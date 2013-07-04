#ifndef LADYVELO_H
#define LADYVELO_H

#include <QtGui>
#include "ui_LadybugVelodyne.h"

class LadybugVelodyne : public QMainWindow
{
	Q_OBJECT

public:
	LadybugVelodyne(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	virtual ~LadybugVelodyne();

private:
	Ui::LadybugVelodyneClass ui;
	QTimer * const timer;

private slots:
	void on_actionOpenVelodyne_triggered();
	void on_actionOpenLadybug_triggered();
	void on_play_clicked();
	void start_play(bool);
	void on_viewer_timeUpdate(int);
};

#endif // LADYVELO_H
