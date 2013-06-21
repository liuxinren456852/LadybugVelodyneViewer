#include "LadybugVelodyne.h"
#include <QtGui>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	LadybugVelodyne w;
	w.show();
	return a.exec();
}
