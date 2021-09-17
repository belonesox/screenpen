#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QtOpenGL/QGLWidget>
#include <QScreen>

namespace Ui {
	class screenwidget;
}

// User data structs.
struct UserObjectData {
	QPoint startPoint;
	QPoint endPoint;
	QPen pen;
};

enum ScreenWidgetState {
	STATE_MOVING,
	STATE_DRAWING,
    STATE_MOVING_AFTER_START_DRAWING,
};

enum ScreenWidgetDrawMode {
	DRAWMODE_ARROW,
	DRAWMODE_LINE,
	DRAWMODE_RECT,
};

class ScreenWidget : public QGLWidget
{
	Q_OBJECT

public:
	explicit ScreenWidget(QWidget *parent = 0);
	~ScreenWidget();

	// void grabDesktop();
	void drawMe();

protected:
	virtual void paintEvent(QPaintEvent *event);

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

	virtual void wheelEvent(QWheelEvent *event);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);

private:
	Ui::screenwidget *ui;

	// Desktop pixmap properties.
	QPixmap		_desktopPixmap;
	QPoint		_desktopPixmapPos;
	QSize		_desktopPixmapSize;
    QScreen*    _desktopScreen;
	float		_desktopPixmapScale;

	// User objects.
	QVector<UserObjectData> _userRects;
	QVector<UserObjectData> _userLines;
    QVector<UserObjectData> _userArrow;
	// Moving properties.
	int		_shiftMultiplier;
	float		_scaleSensivity;


	ScreenWidgetState	_state;
	QPoint		_lastMousePos;


	// Drawing properties.
	ScreenWidgetDrawMode	_drawMode;
	QPoint	_startDrawPoint;
	QPoint	_endDrawPoint;
	QPen	_activePen;

	void shiftPixmap(const QPoint delta);
	void scalePixmapAt(const QPoint pos);

	void checkPixmapPos();

	void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h);
};

#endif // SCREENWIDGET_H
