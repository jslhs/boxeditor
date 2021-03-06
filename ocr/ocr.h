#ifndef OCR_H
#define OCR_H

#include <QtWidgets/QMainWindow>
#include "ui_ocr.h"
#include <QProcess>

class ocr : public QMainWindow
{
	Q_OBJECT

public:
	ocr(QWidget *parent = 0);
	~ocr();

public slots:
	void open(QString img_file);
	void tess_error(QProcess::ProcessError err);
	void tess_started();
	void tess_finished(int code, QProcess::ExitStatus status);
	void show_img();
	void scale_changed(int i);

signals:
	void defer_open(QString img_file);

protected:
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void dropEvent(QDropEvent* ev) override;
	void resizeEvent(QResizeEvent* ev) override;

private:
	void parse(const QString& img_file);

private:
	Ui::ocrClass ui;
	QProcess _tess;
	QImage _img;
};

#endif // OCR_H
