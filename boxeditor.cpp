#include "boxeditor.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QPainter>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QShortCut>

boxeditor::boxeditor(QWidget *parent)
	: QMainWindow(parent)
	, _zoom(1.0f)
{
	ui.setupUi(this);

	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(ui.actionOpenFolder, SIGNAL(triggered()), this, SLOT(open_folder()));
	connect(ui.tblWords, SIGNAL(activated(const QModelIndex&)), this, SLOT(row_activated(const QModelIndex&)));
	connect(ui.tblWords, SIGNAL(clicked(const QModelIndex&)), this, SLOT(row_activated(const QModelIndex&)));
	connect(ui.tblWords, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(item_changed(QTableWidgetItem*)));
	connect(ui.zoom, SIGNAL(currentTextChanged(const QString&)), this, SLOT(zoom_changed(const QString&)));

	ui.box->setParent(this);
	ui.img_view->setWidget(ui.box);

	auto saveKey = new QShortcut(QKeySequence("Ctrl+S"), this);
	connect(saveKey, SIGNAL(activated()), this, SLOT(save()));

	auto tbl = ui.tblWords;
	tbl->verticalHeader()->hide();
	tbl->setColumnWidth(0, 80);
	tbl->setColumnWidth(1, 60);
	tbl->setColumnWidth(2, 60);
	tbl->setColumnWidth(3, 60);
	tbl->setColumnWidth(4, 60);
	tbl->setColumnWidth(5, 60);

	auto act_merge = new QAction("Merge", this);
	auto act_split = new QAction("Split", this);
	auto act_remove = new QAction("Remove", this);
	auto act_add = new QAction("Insert", this);
	auto act_split3 = new QAction("Split 3", this);
	auto act_split4 = new QAction("Split 4", this);

	connect(act_merge, SIGNAL(triggered()), this, SLOT(merge_boxes()));
	connect(act_split, SIGNAL(triggered()), this, SLOT(split_box()));
	connect(act_split3, SIGNAL(triggered()), this, SLOT(split_box3()));
	connect(act_split4, SIGNAL(triggered()), this, SLOT(split_box4()));
	connect(act_remove, SIGNAL(triggered()), this, SLOT(remove_boxes()));
	connect(act_add, SIGNAL(triggered()), this, SLOT(add_box()));

	tbl->addActions({ act_merge, act_split, act_split3, act_split4, act_add, act_remove });

	_status_words = new QLabel("words: 0");
	ui.statusBar->addWidget(_status_words, 50);
}

boxeditor::~boxeditor()
{

}

void boxeditor::open()
{
	QFileDialog dlg(this, "Open Image File");
	dlg.setNameFilter("Image Files (*.bmp *.jpg *.jpeg *.png *.tif *.gif)");
	dlg.setFileMode(QFileDialog::ExistingFile);
	if (dlg.exec() == QDialog::Accepted)
	{
		auto files = dlg.selectedFiles();
		if (files.size())
		{
			open(files.first());
		}
	}
}

void boxeditor::open(const QString& img_file)
{
	setWindowTitle("box editor - " + filename(img_file));
	_img = QImage(img_file);
	_box_filename = img_file;
	auto pos = _box_filename.lastIndexOf(".");
	if (pos != -1)
	{
		_box_filename.replace(pos, _box_filename.length() - pos, ".box");
	}
	else
	{
		_box_filename += ".box";
	}

	auto &boxes = parse_boxes(_box_filename);
	show_boxes(boxes);
	show_img_with_boxes(_img, boxes);
}

void boxeditor::merge_boxes()
{
	auto tbl = ui.tblWords;
	auto ranges = tbl->selectedRanges();
	if (ranges.isEmpty()) return;

	auto range = ranges.first();
	
	QVector<int> lefts, rights, tops, bottoms;
	for (int i = range.topRow(); i <= range.bottomRow(); i++)
	{
		lefts.push_back(tbl->item(i, 1)->text().toInt());
		bottoms.push_back(tbl->item(i, 2)->text().toInt());
		rights.push_back(tbl->item(i, 3)->text().toInt());
		tops.push_back(tbl->item(i, 4)->text().toInt());
	}

	box b;
	auto word = tbl->item(range.topRow(), 0)->text();
	if (word.length()) b.word = word.at(0);
	b.left = *std::min_element(lefts.begin(), lefts.end());
	b.right = *std::max_element(rights.begin(), rights.end());
	b.top = *std::max_element(tops.begin(), tops.end());
	b.bottom = *std::min_element(bottoms.begin(), bottoms.end());
	b.page = tbl->item(range.topRow(), 0)->text().toInt();

	auto &bx = boxes();

	for (int i = range.topRow(); i <= range.bottomRow(); i++)
		bx.remove(range.topRow());

	bx.insert(range.topRow(), b);

	show_boxes(bx);
	show_img_with_boxes(_img, bx);

	tbl->selectRow(range.topRow());
}

void boxeditor::split_box()
{
	split_box(2);
}

void boxeditor::split_box3()
{
	split_box(3);
}

void boxeditor::split_box4()
{
	split_box(4);
}

void boxeditor::remove_boxes()
{

}

void boxeditor::add_box()
{

}

void boxeditor::split_box(int n)
{
	if (n <= 0) return;
	auto tbl = ui.tblWords;
	auto ranges = tbl->selectedRanges();
	if (ranges.isEmpty()) return;
	auto row = ranges.first().topRow();
	auto bx = boxes();
	auto b0 = bx[row];
	int w = b0.width() / n;
	int h = b0.height();
	bx.remove(row);
	for (int i = 0; i < n; i++)
	{
		box b = b0;
		b.left = b0.left + i * w;
		b.right = b.left + w;
		if (i)
		{
			b.left++;
		}
		bx.insert(row + i, b);
	}
	show_boxes(bx);
	show_img_with_boxes(_img, bx);
}

void boxeditor::save()
{
	auto tbl = ui.tblWords;
	QFile file(_box_filename);
	QTextStream ts(&file);
	ts.setCodec("UTF-8");
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
	{
		for (int row = 0; row < tbl->rowCount(); row++)
		{
			auto text = tbl->item(row, 0)->text();
			QChar word = text.isEmpty() ? ' ' : text.at(0);
			ts << word << " "
				<< tbl->item(row, 1)->text() << " "
				<< tbl->item(row, 2)->text() << " "
				<< tbl->item(row, 3)->text() << " "
				<< tbl->item(row, 4)->text() << " "
				<< tbl->item(row, 5)->text() << "\n";
		}
	}

	ui.statusBar->showMessage("box file saved.", 5000);
}

box_list boxeditor::parse_boxes(const QString& filename) const
{
	box_list boxes;
	QFile box_file(filename);
	if (box_file.open(QFile::ReadOnly | QFile::Text))
	{
		QTextStream ts(&box_file);
		ts.setCodec("UTF-8");
		QChar word;
		int left, right, bottom, top, page;
		int row = 0;
		while (!ts.atEnd())
		{
			box b;
			QString text;
			ts >> text >> b.left >> b.bottom >> b.right >> b.top >> b.page;
			b.word = text.isEmpty() ? ' ' : text.at(0);
			ts.read(1);

			boxes.push_back(b);
		}
	}

	return boxes;
}

void boxeditor::show_boxes(const box_list& boxes) const
{
	auto tbl = ui.tblWords;
	tbl->setUpdatesEnabled(false);

	while (tbl->rowCount())
		tbl->removeRow(0);

	for (int i = 0; i < boxes.count(); i++)
	{
		auto &b = boxes[i];
		tbl->insertRow(i);
		tbl->setItem(i, 0, new QTableWidgetItem(QString(b.word)));
		tbl->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(b.left)));
		tbl->setItem(i, 2, new QTableWidgetItem(QString("%1").arg(b.bottom)));
		tbl->setItem(i, 3, new QTableWidgetItem(QString("%1").arg(b.right)));
		tbl->setItem(i, 4, new QTableWidgetItem(QString("%1").arg(b.top)));
		tbl->setItem(i, 5, new QTableWidgetItem(QString("%1").arg(b.page)));
	}

	tbl->setUpdatesEnabled(true);
	tbl->update();

	_status_words->setText(QString("words: %1").arg(boxes.count()));
}

box_list boxeditor::boxes() const
{
	box_list boxes;
	auto tbl = ui.tblWords;
	for (int row = 0; row < tbl->rowCount(); row++)
	{
		box b = {};
		auto item_word = tbl->item(row, 0);
		if (!item_word) break;
		auto item_left = tbl->item(row, 1);
		if (!item_left) break;
		auto item_bottom = tbl->item(row, 2);
		if (!item_bottom) break;
		auto item_right = tbl->item(row, 3);
		if (!item_right) break;
		auto item_top = tbl->item(row, 4);
		if (!item_top) break;
		auto item_page = tbl->item(row, 5);
		if (!item_page) break;

		auto word = item_word->text();
		if (word.length()) b.word = word.at(0);
		b.left = item_left->text().toInt();
		b.bottom = item_bottom->text().toInt();
		b.right = item_right->text().toInt();
		b.top = item_top->text().toInt();
		b.page = item_page->text().toInt();
		boxes.push_back(b);
	}

	return boxes;
}

void boxeditor::save(const QString& filename, const box_list& boxes) const
{

}

void boxeditor::show_img_with_boxes(const QImage& img, const box_list& boxes) const
{
	QImage draw_img(img.scaled(img.width() * _zoom, img.height() * _zoom));
	auto ranges = ui.tblWords->selectedRanges();

	auto inRange = [&](int row){
		for (auto range : ranges)
		{
			if (row >= range.topRow() && row <= range.bottomRow())
				return true;
		}
		return false;
	};

	int i = 0;
	QPainter p(&draw_img);
	QPen pen(Qt::red);

	for (auto box : boxes)
	{
		pen.setWidth(inRange(i) ? 3 : 1);
		p.setPen(pen);
		p.drawRect(box.left * _zoom, (img.height() - box.top) * _zoom, _zoom * box.width(), _zoom * box.height());
		i++;
	}

	ui.box->setPixmap(QPixmap::fromImage(draw_img));
}

void boxeditor::row_activated(const QModelIndex & index)
{
	show_img_with_boxes(_img, boxes());
}

void boxeditor::item_changed(QTableWidgetItem * item)
{
	show_img_with_boxes(_img, boxes());
}

void boxeditor::dropEvent(QDropEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		ev->accept();
		auto urls = ev->mimeData()->urls();
		if (urls.isEmpty()) return;
		auto filename = urls.first().toLocalFile();
		QDir dir(filename);
		if (dir.exists()) open(dir);
		else
			open(filename);
	}
}

void boxeditor::dragEnterEvent(QDragEnterEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		ev->accept();

		bool can_drop = false;
		auto &urls = ev->mimeData()->urls();
		if (urls.isEmpty() || (urls.count() > 1))
		{
			can_drop = false;
		}
		else
		{
			auto filename = urls.first().toLocalFile();
			QDir dir(filename);
			QRegExp exp(".*[jpg|jpeg|bmp|tif|png|gif]$");
			can_drop = dir.exists() || exp.exactMatch(filename);
		}

		ev->setDropAction(can_drop ? Qt::LinkAction : Qt::IgnoreAction);
	}
}

QString boxeditor::filename(const QString& path) const
{
	auto &tokens = path.split('/');
	return tokens.isEmpty() ? "" : tokens.last();
}

void boxeditor::change_img()
{
	auto s = dynamic_cast<QAction*>(sender());
	if (s)
	{
		_zoom = 1.0f;
		ui.zoom->setCurrentText("100%");
		open(s->data().value<QString>());
	}

	for (auto act : _img_acts)
	{
		act->setChecked(s == act);
	}
}

void boxeditor::add_imgs(const QStringList& imgs)
{
	auto menu = ui.menuImages;
	for (auto act : _img_acts)
	{
		menu->removeAction(act);
		disconnect(act, SIGNAL(triggered()), this, SLOT(change_img()));
	}
	_img_acts.clear();

	if (imgs.isEmpty()) return;

	for (auto &img : imgs)
	{
		auto act = new QAction(filename(img), this);
		act->setData(QVariant(img));
		act->setCheckable(true);
		connect(act, SIGNAL(triggered()), this, SLOT(change_img()));
		_img_acts.push_back(act);
	}

	_img_acts.first()->setChecked(true);
	open(imgs.first());

	menu->addActions(_img_acts);
}

void boxeditor::open(QDir dir)
{
	QStringList filters;
	filters << "*.jpg" << "*.jpeg" << "*.bmp" << "*.png" << "*.tif" << "*.gif";
	if (dir.exists())
	{
		auto fileInfos = dir.entryInfoList(filters, QDir::Files);
		QStringList files;
		for (auto &fileInfo : fileInfos)
		{
			files.push_back(fileInfo.filePath());
		}

		add_imgs(files);
	}
}

void boxeditor::open_folder()
{
	QFileDialog dlg(this, "Choose Folder");
	dlg.setFilter(QDir::Dirs);
	dlg.setFileMode(QFileDialog::Directory);
	if (dlg.exec() == QDialog::Accepted)
	{
		auto &folders = dlg.selectedFiles();
		if (folders.isEmpty()) return;
		else
			open(QDir(folders.first()));
	}
}

void boxeditor::zoom_changed(const QString& text)
{
	auto t = text;
	bool ok = false;
	t.remove("%");
	auto val = t.toInt(&ok);
	if (!ok) return;
	_zoom = val / 100.0f;
	if (!text.endsWith("%")) 
		ui.zoom->setCurrentText(text + "%");
	show_img_with_boxes(_img, boxes());
}
