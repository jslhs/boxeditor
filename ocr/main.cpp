#include "ocr.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	ocr w;
	w.show();
	return a.exec();
}
