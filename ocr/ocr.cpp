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

	ui.img_view->setParent(this);
	ui.img_view->setText("Drop Image Here.");
	ui.scrollArea->setWidget(ui.img_view);

	connect(this, SIGNAL(defer_open(QString)), this, SLOT(open(QString)), Qt::QueuedConnection);
	connect(&_tess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(tess_error(QProcess::ProcessError)));
	connect(&_tess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(tess_finished(int, QProcess::ExitStatus)));
	connect(&_tess, SIGNAL(started()), this, SLOT(tess_started()));

	auto dir = QDir::currentPath();

	QStringList env;
	env << QString("TESSDATA_PREFIX=%1").arg(dir);
	_tess.setProgram("bin/tesseract.exe");
	_tess.setEnvironment(env);
	_tess.setStandardOutputFile("out.txt");
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
	ui.img_view->setPixmap(QPixmap::fromImage(QImage(img_file)));
	QStringList args;
	args << img_file << "result" << "-l" << "chi_demo2";
	_tess.setArguments(args);
	_tess.start();
}

void ocr::tess_error(QProcess::ProcessError err)
{

}

void ocr::tess_started()
{

}

void ocr::tess_finished(int code, QProcess::ExitStatus status)
{
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
					ui.txt->setText(txt.mid(s, e - s));
					return;
				}
			}
		}
	}

	ui.txt->setText("ERROR!");
}
