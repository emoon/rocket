#include "trackview.h"

#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QKeyEvent>
#include <QtGui/QScrollBar>
#include <QtGui/QHeaderView>

TrackViewInner::TrackViewInner(QWidget *parent) :
	QWidget(parent),
	font("Fixedsys"),
	currRow(0),
	currCol(0)
{
	QFontMetrics fm(font);
	setFixedWidth(fm.width(' ') * 8 * 3);
	setFixedHeight(fm.lineSpacing() * 32);
}

TrackView::TrackView(QWidget *parent) :
	QScrollArea(parent)
{
	inner = new TrackViewInner(this);
	setWidget(inner);
	setAlignment(Qt::AlignCenter);
}


void TrackViewInner::setRow(int row)
{
	row = qMax(row, 0);
	if (currRow != row) {
		currRow = row;
		update();
	} else
		QApplication::beep();
}

void TrackViewInner::setCol(int col)
{
	col = qMax(col, 0);
	if (currCol != col) {
		currCol = col;
		update();
	} else
		QApplication::beep();
}

void TrackView::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Up:
		inner->setRow(inner->getRow() - 1);
		break;

	case Qt::Key_Down:
		inner->setRow(inner->getRow() + 1);
		break;

	case Qt::Key_PageUp:
		inner->setRow(inner->getRow() - 16);
		break;

	case Qt::Key_PageDown:
		inner->setRow(inner->getRow() + 16);
		break;

	case Qt::Key_Left:
		inner->setCol(inner->getCol() - 1);
		break;

	case Qt::Key_Right:
		inner->setCol(inner->getCol() + 1);
		break;
	}
}

void TrackViewInner::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);

	painter.fillRect(0, 0, width(), height(), palette().background());

	painter.setFont(font);

	for (int c = 0; c < 3; ++c) {

		QRect rect;
		rect.setLeft(c * painter.fontMetrics().width(' ') * 8);
		rect.setRight((c + 1) * painter.fontMetrics().width(' ') * 8);

		for (int r = 0; r < 32; ++r) {
			rect.setTop(r * painter.fontMetrics().lineSpacing());
			rect.setBottom((r + 1) * painter.fontMetrics().lineSpacing());

			if (c == currCol && r == currRow)
				painter.fillRect(rect, palette().highlight());
			else
				painter.fillRect(rect, r % 8 ?
					palette().base() :
					palette().alternateBase());

			QString temp = QString("---");
			if (!(r % 4))
				temp.setNum(float(r) / 8, 'f', 4);
			painter.drawText(rect, temp);
		}
	}
}
