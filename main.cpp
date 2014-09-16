#include "boxeditor.h"
#include <QtWidgets/QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	boxeditor w;
	w.show();
	QApplication::setStyle(QStyleFactory::create("Fusion"));
	return a.exec();
}
