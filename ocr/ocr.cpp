#include "ocr.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QTextStream>

ocr::ocr(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowTitle("Text Recognition");

	ui.img_view->setParent(this);
	ui.scrollArea->setWidget(ui.img_view);

	connect(this, SIGNAL(defer_open(QString)), this, SLOT(open(QString)), Qt::QueuedConnection);
	connect(&_tess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(tess_error(QProcess::ProcessError)));
	connect(&_tess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(tess_finished(int, QProcess::ExitStatus)));
	connect(&_tess, SIGNAL(started()), this, SLOT(tess_started()));
	connect(ui.scale, SIGNAL(currentIndexChanged(int)), this, SLOT(scale_changed(int)));

	auto dir = QDir::currentPath();

	QStringList env;
	env << QString("TESSDATA_PREFIX=%1").arg(dir);
	_tess.setProgram("bin/tesseract.exe");
	_tess.setEnvironment(env);
	//_tess.setStandardOutputFile("out.txt");
	_tess.setStandardErrorFile("error.txt");
}

ocr::~ocr()
{

}

void ocr::dragEnterEvent(QDragEnterEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		auto &urls = ev->mimeData()->urls();
		auto &url = urls.first();
		auto &filename = url.toLocalFile();
		bool can_drop = false;
		QFile file(filename);
		QRegExp exp(".*[jpg|jpeg|bmp|tif|png|gif]$");
		can_drop = exp.exactMatch(filename)
			&& file.exists();
		ev->setDropAction(can_drop ? Qt::LinkAction : Qt::IgnoreAction);
		ev->accept();
	}
}

void ocr::dropEvent(QDropEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		ev->accept();
		defer_open(ev->mimeData()->urls().first().toLocalFile());
	}
}

void ocr::open(QString img_file)
{
	_img = QImage(img_file);
	QStringList args;
	args << img_file << "result" << "-l" << "chi_demo2";
	_tess.setArguments(args);
	_tess.start();
	show_img();
}

void ocr::show_img()
{
	if (_img.isNull())
	{
		ui.img_view->setText("Drop Image Here.");
		return;
	}

	int i = ui.scale->currentIndex();
	QImage img;
	if (i == 0)
	{
		int w = ui.scrollArea->geometry().width();
		int h = ui.scrollArea->geometry().height();
		img = _img.scaled(w, h, Qt::KeepAspectRatio);
	}
	else if (i == 1)
	{
		img = _img;
	}

	ui.img_view->setPixmap(QPixmap::fromImage(img));
}

void ocr::tess_error(QProcess::ProcessError err)
{
	ui.txt->setEnabled(true);
	ui.txt->setText("ERROR!");
}

void ocr::tess_started()
{
	ui.txt->setEnabled(false);
	ui.txt->setText("Analyzing, please wait...");
}

void ocr::tess_finished(int code, QProcess::ExitStatus status)
{
	ui.txt->setEnabled(true);
	if (!code)
	{
		QFile file("result.txt");
		if (file.open(QIODevice::ReadOnly))
		{
			QTextStream ts(&file);
			ts.setCodec("UTF-8");
			auto &txt = ts.readAll();
			auto s = txt.indexOf(QStringLiteral("µçÉó"));
			if (s != -1)
			{
				auto e = txt.indexOf(QStringLiteral("ºÅ"), s);
				if (e != -1)
				{
					ui.txt->setText(txt.mid(s, e - s + 1));
					return;
				}
			}
		}
	}

	ui.txt->setText("ERROR!");
}

void ocr::scale_changed(int i)
{
	show_img();
}

void ocr::resizeEvent(QResizeEvent* ev)
{
	show_img();
}