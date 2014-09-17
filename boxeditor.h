#ifndef BOXEDITOR_H
#define BOXEDITOR_H

#include <QtWidgets/QMainWindow>
#include "ui_boxeditor.h"

#include <QVector>
#include <QImage>
#include <QDir>

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
	void open_folder();
	void save();
	void row_activated(const QModelIndex & index);
	void item_changed(QTableWidgetItem * item);

	void merge_boxes();
	void split_box();
	void split_box3();
	void split_box4();
	void remove_boxes();
	void add_box();
	void change_img();
	void zoom_changed(const QString& text);

protected:
	void dropEvent(QDropEvent* ev) override;
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void open(const QString& img_file);
	box_list parse_boxes(const QString& filename) const;
	void show_boxes(const box_list& boxes) const;
	box_list boxes() const;
	void save(const QString& filename, const box_list& boxes) const;
	void show_img_with_boxes(const QImage& img, const box_list& boxes) const;
	QString filename(const QString& path) const;
	void add_imgs(const QStringList& imgs);
	void open(QDir dir);
	void split_box(int n);

private:
	Ui::boxeditorClass ui;

	QList<QAction*> _img_acts;
	QString _box_filename;
	QImage _img;
	QLabel* _status_words;
	float _zoom;
};

#endif // BOXEDITOR_H
