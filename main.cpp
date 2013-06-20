#include "LadybugVelodyne.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	LadybugVelodyne w;
	w.show();
	return a.exec();
}
