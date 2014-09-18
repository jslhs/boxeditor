#ifndef OCR_H
#define OCR_H

#include <QtWidgets/QMainWindow>
#include "ui_ocr.h"

class ocr : public QMainWindow
{
	Q_OBJECT

public:
	ocr(QWidget *parent = 0);
	~ocr();

private:
	Ui::ocrClass ui;
};

#endif // OCR_H
