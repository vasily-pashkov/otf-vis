#ifndef GRAPHER_H
#define GRAPHER_H

#include "stand_trace_model.h"

#include <QDialog>
#include <QFrame>
#include <QVariant>
#include <QMultiHash>
#include <QTime>
#include <QSettings>
#include <QFileInfo>

class QPushButton;

namespace vis3 {

namespace stand {

class ParamInfoPopup;
class ParamsFilterWidget;

class Stand_update_event;

typedef QList<Stand_update_event*> Stand_update_event_list;

/** Graph window class. */
class StandGrapher : public QDialog {

    Q_OBJECT

public: /* constants */

    /** @name Margins of the drawing area. */
    //@{
    static const int axis_x_margin = 30;
    static const int axis_y_margin = 30;
    //@}

    /** Diameter of including area about the cursor. */
    static const int mouseLookWidth = 10;

    /** Point's hash cell size. */
    static const int hashCellSize = 50;

public: /* methods */

    StandGrapher(QWidget * parent = 0);
    void restore_scope();
    void setPortableDrawing(bool);

private: /* methods */
    void prepare_settings(QSettings& settings, const Trace_model::Ptr & model) const;
    QString canonicalTraceId(const QString& trc_file_path) const;
    void save_geometry() const;
    void restore_geometry();

    void save_scope() const;

    /** Event filter. */
    bool eventFilter(QObject * obj, QEvent * event);

    void createGui();
    void createCanvas();

    /** Disables unavailable toolbar's actions. */
    void checkActions();

    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);

    void wheelEvent(QWheelEvent * event);

    void moveToolBar(QFrame * toolbar, const QPoint & movement);

    void drawAxis(QPainter * p, Qt::Orientation orientation);
    void needRedraw(bool recreateCanvas = false);

    int pixelPositionForTime(const vis3::common::Time& time) const;
    vis3::common::Time timeForPixel(int pixel_x) const;

    QColor paramColor(int param) const;

    /** Hash-function. Uses graph coordinate system */
    uint hash(int x, int y);

    /** Hash-function. Uses window coordinate system */
    uint hash(const QPoint & p);

    /** Returns points about the position p */
    void pointsAbout(const QPoint & p, Stand_update_event_list & targetList);

private: /* members */

    Stand_trace_model::Ptr model_;

    bool initialized;
    bool portable_drawing;
    bool drawing_needed;
    bool antialiasing;

    std::auto_ptr<QPaintDevice> canvas_;
    int area_width;
    int area_height;

    double min_value;
    double max_value;

    /** @name These using for vertical fitting. */
    //@{
    double min_value_for_fit;
    double max_value_for_fit;
    bool alreadyFitted;
    //@}

    QMultiHash<int, boost::shared_ptr<Stand_update_event> > pointsHash;

private: /* widgets */

    QCursor * dotboxCursor;
    ParamInfoPopup * paramWnd;

    QFrame * hToolBox;
    QFrame * vToolBox;
    QFrame * toolBox;

    QPushButton * printButton;

    QPushButton * filterButton;
    ParamsFilterWidget * filterWindow;

    QPushButton * drawLinesButton;
    QPushButton * antialiasingButton;

public slots:

    void setModel(Stand_trace_model::Ptr model);
    void setModel(Trace_model::Ptr model);

private slots:

    void drawGraph(QPainter * p = 0);

    void slotStart();
    void slotPrevious();
    void slotFit();
    void slotNext();
    void slotEnd();

    void slotZoomIn();
    void slotZoomOut();

    void slotUp();
    void slotVFit();
    void slotDown();

    void slotVZoomIn();
    void slotVZoomOut();

    void slotFilterWindow();
    void slotDrawLineSwither();
    void slotAntialiasingSwither(bool);
    void slotPrintGraph();

private: /* support for background drawing */

    /** Active drawing flag. */
    bool drawing_in_progress;

    /** Periodically updates canvas and show current state of drawing. */
    void timerEvent(QTimerEvent * event);

    /** Painter timer. Involves repaint for showing intermediate results of drawing. */
    int painter_timer;
    int painter_timer_interval;

    // Used for testing real time interval, that
    // elapsed since previous timer tick.
    // If real interval greater, try to select
    // more approprite inverval.
    QTime painter_timer_supervisor;

};

}

}
#endif
