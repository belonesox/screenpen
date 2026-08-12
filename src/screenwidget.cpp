#include "screenwidget.h"
#include "ui_screenwidget.h"

#include <QtGui/QPainter>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QScreen>
#include <QtDebug>
#include <cmath>

ScreenWidget::ScreenWidget(QWidget *parent) :
        QGLWidget(parent),
        ui(new Ui::screenwidget)
{
    // this->initializeGL();
    QPoint globalCursorPos_ = QCursor::pos();

    // tmp for debug
    //globalCursorPos_ = QPoint(1930, 400);

    _desktopScreen = QGuiApplication::screenAt(globalCursorPos_);
    QRect msg(_desktopScreen->geometry());


    QPoint topLeft(msg.topLeft());
    QSize winSize(msg.size());
    _desktopPixmapPos = QPoint(0,0);
    _desktopPixmapSize = winSize;


    int x(topLeft.x());
    int w(winSize.width() - 1);
    int y(topLeft.y());
    int h(winSize.height() - 1);
    WId wid_ (QApplication::desktop()->winId());

    _desktopPixmap = _desktopScreen->grabWindow(wid_, x, y, w, h);    

    ui->setupUi(this);
    setMouseTracking(true);

    _state = STATE_MOVING;

    _desktopPixmapScale = 1.0f;

    _shiftMultiplier = 2;
    _scaleSensivity = 0.1f;

   _drawMode = DRAWMODE_ARROW;

    _activePen.setColor(QColor(255, 0, 0));
    _activePen.setWidth(4);


    resize(w+1, h+1);
    move(x, y); // Not working when Qt::FramelessWindowHint without Qt::X11BypassWindowManagerHint
    setWindowFlags(
        Qt::FramelessWindowHint 
            | 
        Qt::X11BypassWindowManagerHint            
            |
        Qt::Tool 
            | 
        Qt::WindowStaysOnTopHint
        );
    show();
    activateWindow(); // Need because of Qt::X11BypassWindowManagerHint
}

ScreenWidget::~ScreenWidget()
{
    delete ui;
}

void ScreenWidget::drawMe()
{
    QPainter p;

    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw desktop pixmap.
    p.drawPixmap(0, 0,
            width(),
            height(),
            _desktopPixmap,
             _desktopPixmapPos.x(), _desktopPixmapPos.y(),
             _desktopPixmapSize.width(), _desktopPixmapSize.height()
             );

    // Draw user objects.
    int x, y, w, h;
    for (int i = 0; i < _userRects.size(); ++i) {
        p.setPen(_userRects.at(i).pen);
        getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h);
        p.drawRect(x, y, w, h);
    }

    // Draw user lines.
    for (int i = 0; i < _userLines.size(); ++i) {
        getRealUserObjectPos(_userLines.at(i), &x, &y, &w, &h);
        drawArrow(p, x, y, x+w, y+h, _userLines.at(i).pen);
    }

    // Draw user arrow.
    for (int i = 0; i < _userArrow.size(); ++i) {
        p.setPen(_userArrow.at(i).pen);
        getRealUserObjectPos(_userArrow.at(i), &x, &y, &w, &h);
        p.drawLine(x, y, x+w, y+h);
    }

    // Draw active user object.
    if (_state == STATE_DRAWING) {
        p.setPen(_activePen);
        points2xywh(_startDrawPoint, _endDrawPoint, &x, &y, &w, &h);

        if (_drawMode == DRAWMODE_RECT) {
            p.drawRect(x, y, w, h);
        } else if (_drawMode == DRAWMODE_LINE) {
            drawArrow(p, x, y, w + x, h + y, _activePen);
        } else if (_drawMode == DRAWMODE_ARROW) {
            p.drawPoint(x, y);
        }
    }

    p.end();
}


void ScreenWidget::paintEvent(QPaintEvent *event)
{
    drawMe();
}

void ScreenWidget::mousePressEvent(QMouseEvent *event)
{
    _lastMousePos = event->pos();

    _state = STATE_DRAWING;

    // _startDrawPoint = (_lastMousePos - _desktopPixmapPos)/_desktopPixmapScale;
    _startDrawPoint = coordScreen2Pixmap(_lastMousePos);
    // _startDrawPoint = _lastMousePos /_desktopPixmapScale;
    _endDrawPoint = _startDrawPoint;
}

void ScreenWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (_state == STATE_DRAWING) {
        QPoint pos(event->pos());
        _endDrawPoint = coordScreen2Pixmap(pos);
        // _endDrawPoint = (pos - _desktopPixmapPos)/_desktopPixmapScale;
        // _endDrawPoint = pos / _desktopPixmapScale;

        UserObjectData data;
        data.pen = _activePen;
        data.startPoint = _startDrawPoint;
        data.endPoint = _endDrawPoint;
        if (_drawMode == DRAWMODE_LINE) {
            _userLines.append(data);
        } else if (_drawMode == DRAWMODE_RECT) {
            _userRects.append(data);
        } else if (_drawMode == DRAWMODE_ARROW) {
            _userArrow.append(data);
        }

        _state = STATE_MOVING_AFTER_START_DRAWING;
        update();
    }
}

void ScreenWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (_state == STATE_MOVING) {
        QPoint delta = event->pos() - _lastMousePos;

        // shiftPixmap(delta);
        checkPixmapPos();

    } else if (_state == STATE_DRAWING) {
        _endDrawPoint = coordScreen2Pixmap(event->pos());
        // _endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
        if (_drawMode == DRAWMODE_ARROW) {
            UserObjectData dataArrow;
            dataArrow.pen = _activePen;
            dataArrow.startPoint = _startDrawPoint;
            dataArrow.endPoint = _endDrawPoint;

            _userArrow.append(dataArrow);
            _startDrawPoint = _endDrawPoint;
        }
    }

    _lastMousePos = event->pos();
    update();
}

void ScreenWidget::wheelEvent(QWheelEvent *event)
{
    if ((_state == STATE_MOVING) || (_state == STATE_MOVING_AFTER_START_DRAWING)) {
        const int delta = event->angleDelta().y();
        if (delta == 0) {
            // Purely horizontal wheel movement, nothing to zoom.
            return;
        }
        const int sign = (delta > 0) ? 1 : -1;

        _desktopPixmapScale += sign * _scaleSensivity;
        if (_desktopPixmapScale < 1.0f) {
            _desktopPixmapScale = 1.0f;
        }

        scalePixmapAt(event->position().toPoint());
        checkPixmapPos();

        update();
    }
}

void ScreenWidget::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    if (key == Qt::Key_Escape ||
            key == Qt::Key_Tab) {
        QApplication::quit();
    } else if ((key >= Qt::Key_1) && (key <= Qt::Key_9)) {
        _activePen.setWidth(key - Qt::Key_0);
    } else if (key == Qt::Key_R) {
        _activePen.setColor(QColor(255, 0, 0));
    } else if (key == Qt::Key_G) {
        _activePen.setColor(QColor(0, 255, 0));
    } else if (key == Qt::Key_B) {
        _activePen.setColor(QColor(0, 0, 255));
    } else if (key == Qt::Key_C) {
        _activePen.setColor(QColor(0, 255, 255));
    } else if (key == Qt::Key_M) {
        _activePen.setColor(QColor(255, 0, 255));
    } else if (key == Qt::Key_Y) {
        _activePen.setColor(QColor(255, 255, 0));
    } else if (key == Qt::Key_W) {
        _activePen.setColor(QColor(255, 255, 255));
    } else if (key == Qt::Key_Q) {
        _userRects.clear();
        _userLines.clear();
        _userArrow.clear();
        _state = STATE_MOVING;
    } else if (key == Qt::Key_Z) {
        _drawMode = DRAWMODE_LINE;
    } else if (key == Qt::Key_X) {
        _drawMode = DRAWMODE_RECT;
    } else if (key == Qt::Key_A) {
        _drawMode = DRAWMODE_ARROW;
    }

    update();
}

void ScreenWidget::keyReleaseEvent(QKeyEvent *event)
{
}


void ScreenWidget::shiftPixmap(const QPoint delta)
{
    // _desktopPixmapPos -= delta * _shiftMultiplier;
}

void ScreenWidget::scalePixmapAt(const QPoint pos)
{
    int old_w = _desktopPixmapSize.width();
    int old_h = _desktopPixmapSize.height();

    int new_w = _desktopPixmap.width() / _desktopPixmapScale;
    int new_h = _desktopPixmap.height() / _desktopPixmapScale;
    _desktopPixmapSize = QSize(new_w, new_h);

    // int dw = new_w - old_w;
    // int dh = new_h - old_h;

    // int cur_x = pos.x() + abs(_desktopPixmapPos.x());
    // int cur_y = pos.y() + abs(_desktopPixmapPos.y());

    // float cur_px = -((float)cur_x / old_w);
    // float cur_py = -((float)cur_y / old_h);
    _desktopPixmapPos.setX((int)pos.x() * (_desktopPixmapScale-1)/_desktopPixmapScale);
    _desktopPixmapPos.setY((int)pos.y() * (_desktopPixmapScale-1)/_desktopPixmapScale);
    // _desktopPixmapPos.setY(_desktopPixmapPos.y() + dh*cur_py);

    // _desktopPixmapPos.setX(_desktopPixmapPos.x() + dw*cur_px);
    // _desktopPixmapPos.setY(_desktopPixmapPos.y() + dh*cur_py);
    checkPixmapPos();
}

void ScreenWidget::checkPixmapPos()
{

    if (_desktopPixmapPos.x() < 0){
        _desktopPixmapPos.setX(0);
    }

    if (_desktopPixmapPos.x() > width()-1){
        _desktopPixmapPos.setX(width()-1);
    }

    if (_desktopPixmapPos.y() < 0){
        _desktopPixmapPos.setY(0);
    }

    if (_desktopPixmapPos.y() > height()-1){
        _desktopPixmapPos.setY(height()-1);
    }

    if (_desktopPixmapPos.x() + _desktopPixmapSize.width() > width()){
        _desktopPixmapSize.setWidth(width() - _desktopPixmapPos.x());
    }

    if (_desktopPixmapPos.y() + _desktopPixmapSize.height() > height()){
        _desktopPixmapSize.setHeight(height() - _desktopPixmapPos.y());
    }

    // if (_desktopPixmapPos.x() > 0) {
    //     _desktopPixmapPos.setX(0);
    // } else if ((_desktopPixmapSize.width() + _desktopPixmapPos.x()) < width()) {
    //     _desktopPixmapPos.setX(width() - _desktopPixmapSize.width());
    // }

    // if (_desktopPixmapPos.y() > 0) {
    //     _desktopPixmapPos.setY(0);
    // } else if ((_desktopPixmapSize.height() + _desktopPixmapPos.y()) < height()) {
    //     _desktopPixmapPos.setY(height() - _desktopPixmapSize.height());
    // }
}

const QPoint ScreenWidget::coordScreen2Pixmap(const QPoint& pos){
    const QPoint res(_desktopPixmapPos + pos/_desktopPixmapScale);
    // qInfo() << "coordScreen2Pixmap" << pos << " to " << res;
    return res;
};

const QPoint ScreenWidget::coordPixmap2Screen(const QPoint& pos){
    const QPoint res((pos - _desktopPixmapPos)*_desktopPixmapScale);
    // qInfo() << "coordPixmap2Screen" << pos << " to " << res;
    return res;
};


void ScreenWidget::points2xywh(const QPoint &start_point, const QPoint &end_point, int *x, int *y, int *w, int *h)
{
    QPoint screen_start(coordPixmap2Screen(start_point));
    QPoint screen_end(coordPixmap2Screen(end_point));

    *x = screen_start.x();
    *y = screen_start.y();
    // coordPixmap2Screen() already applies _desktopPixmapScale, so the delta
    // between two converted points must not be scaled a second time.
    *w = screen_end.x() - screen_start.x();
    *h = screen_end.y() - screen_start.y();
}



void ScreenWidget::getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h)
{
    points2xywh(userObj.startPoint, userObj.endPoint, x, y, w, h);
}

// Draws a line from (x1, y1) to (x2, y2) ending in a filled arrowhead.
void ScreenWidget::drawArrow(QPainter &p, int x1, int y1, int x2, int y2, const QPen &pen)
{
    const qreal penWidth = qMax(qreal(1), pen.widthF());

    const QPointF tail(x1, y1);
    const QPointF tip(x2, y2);
    const qreal lineLen = QLineF(tail, tip).length();
    if (lineLen < 1) {
        return;
    }

    // Head proportions: a slim 4:3 triangle whose back edge is notched inwards,
    // which reads better than a plain triangle at any pen width.
    const qreal notchRatio = 0.75;
    qreal headLen = qMax(qreal(12), penWidth * 4);
    // Never let the head eat the whole line.
    headLen = qMin(headLen, lineLen * qreal(0.9));
    const qreal halfHeadWidth = headLen * qreal(0.375);

    const QPointF dir = (tip - tail) / lineLen;
    const QPointF normal(-dir.y(), dir.x());

    const QPointF backCenter = tip - dir * headLen;
    const QPointF notch = tip - dir * (headLen * notchRatio);

    QPolygonF head;
    head << tip
         << backCenter + normal * halfHeadWidth
         << notch
         << backCenter - normal * halfHeadWidth;

    p.save();

    // Stop the shaft at the notch so it does not show through the head.
    const qreal shaftLen = lineLen - headLen * notchRatio;
    if (shaftLen > 0) {
        QPen shaftPen(pen);
        shaftPen.setCapStyle(Qt::RoundCap);
        p.setPen(shaftPen);
        p.drawLine(QLineF(tail, notch));
    }

    // Fill the head and outline it with a hairline of the same colour so the
    // sharp tip stays crisp instead of fading out through antialiasing.
    QPen headPen(pen.color());
    headPen.setWidthF(1);
    headPen.setJoinStyle(Qt::MiterJoin);
    p.setPen(headPen);
    p.setBrush(pen.color());
    p.drawPolygon(head);

    p.restore();
}
