#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QtGui/QWidget>
#include <QtGui/QScrollArea>

class TrackViewInner : public QWidget {
	Q_OBJECT

public:
	TrackViewInner(QWidget *parent = 0);

	int getRow() { return currRow; }
	void setRow(int row);
	int getCol() { return currCol; }
	void setCol(int col);

protected:
	void paintEvent(QPaintEvent *event);

	QFont font;
	int currRow, currCol;
};

class TrackView : public QScrollArea {
	Q_OBJECT

public:
	TrackView(QWidget *parent = 0);

protected:
	void keyPressEvent(QKeyEvent *event);
	TrackViewInner *inner;
};

#endif // TRACKVIEW_H
