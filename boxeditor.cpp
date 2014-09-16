#include "boxeditor.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QPainter>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

boxeditor::boxeditor(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(ui.tblWords, SIGNAL(activated(const QModelIndex&)), this, SLOT(row_activated(const QModelIndex&)));
	connect(ui.tblWords, SIGNAL(clicked(const QModelIndex&)), this, SLOT(row_activated(const QModelIndex&)));
	connect(ui.tblWords, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(item_changed(QTableWidgetItem*)));

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
	auto act_add = new QAction("Add", this);

	connect(act_merge, SIGNAL(triggered()), this, SLOT(merge_boxes()));
	connect(act_split, SIGNAL(triggered()), this, SLOT(split_box()));
	connect(act_remove, SIGNAL(triggered()), this, SLOT(remove_boxes()));
	connect(act_add, SIGNAL(triggered()), this, SLOT(add_box()));

	tbl->addActions({ act_merge, act_split, act_add, act_remove });
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

}

void boxeditor::remove_boxes()
{

}

void boxeditor::add_box()
{

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
			ts << tbl->item(row, 0)->text() << " "
				<< tbl->item(row, 1)->text() << " "
				<< tbl->item(row, 2)->text() << " "
				<< tbl->item(row, 3)->text() << " "
				<< tbl->item(row, 4)->text() << " "
				<< tbl->item(row, 5)->text() << "\n";
		}
	}
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
			ts >> b.word >> b.left >> b.bottom >> b.right >> b.top >> b.page;
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
	QImage draw_img(img);
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
		p.drawRect(box.left, img.height() - box.top, box.width(), box.height());
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
			QRegExp exp(".*[jpg|jpeg|bmp|tif|png|gif]$");
			can_drop = exp.exactMatch(filename);
		}

		ev->setDropAction(can_drop ? Qt::LinkAction : Qt::IgnoreAction);
	}
}
