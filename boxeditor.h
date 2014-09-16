#ifndef BOXEDITOR_H
#define BOXEDITOR_H

#include <QtWidgets/QMainWindow>
#include "ui_boxeditor.h"

#include <QVector>
#include <QImage>

struct box
{
	QChar word;
	int left;
	int bottom;
	int right;
	int top;
	int page;
	
	int width() const
	{
		return right - left;
	}

	int height() const
	{
		return top - bottom;
	}
};

typedef QVector<box> box_list;

class boxeditor : public QMainWindow
{
	Q_OBJECT

public:
	boxeditor(QWidget *parent = 0);
	~boxeditor();

public slots:
	void open();
	void save();
	void row_activated(const QModelIndex & index);
	void item_changed(QTableWidgetItem * item);

	void merge_boxes();
	void split_box();
	void remove_boxes();
	void add_box();
	
protected:
	void dropEvent(QDropEvent* ev) override;
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void open(const QString& img_file);
	box_list parse_boxes(const QString& filename) const;
	void show_boxes(const box_list& boxes) const;
	box_list boxes() const;
	void save(const QString& filename, const box_list& boxes) const;
	void show_img_with_boxes(const QImage& img, const box_list& boxes) const;

private:
	Ui::boxeditorClass ui;

	QString _box_filename;
	QImage _img;
};

#endif // BOXEDITOR_H
