#ifndef LADYVELO_H
#define LADYVELO_H

#include <QtGui>
#include "ui_LadybugVelodyne.h"

class LadybugVelodyne : public QMainWindow
{
	Q_OBJECT

public:
	LadybugVelodyne(QWidget *parent = 0, Qt::WFlags flags = 0);
	virtual ~LadybugVelodyne();

private:
	Ui::LadybugVelodyneClass ui;
};

#endif // LADYVELO_H
