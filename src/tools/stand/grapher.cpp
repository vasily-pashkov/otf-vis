#include "grapher.h"
#include "stand_trace_model.h"
#include "stand_event_model.h"
#include "paraminfo.h"
#include "params_filter.h"
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QHash>
#include <QIcon>
#include <QHBoxLayout>
#include <QApplication>
#include <QMouseEvent>
#include <QBitmap>
#include <QToolTip>
#include <QSettings>
#include <QDesktopWidget>
#include <QPushButton>
#include <QSet>
#include <QMenu>
#include <QShortcut>
#include <QApplication>
#include <QTimer>
#include <cmath>
#define REDRAW_GRAPH    needRedraw(false);
#define REDRAW_WINDOW   needRedraw(true);

namespace vis3{  namespace stand {

using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using vis3::common::Time;
using vis3::common::Selection;

StandGrapher::StandGrapher(QWidget * parent)
    : QDialog(parent, Qt::Window), initialized(false),
      portable_drawing(false), drawing_needed(true),
      antialiasing(true), canvas_(0), min_value(-20.0),
      max_value(20.0), paramWnd(0), drawing_in_progress(false),
      painter_timer_interval(100)
{
    setWindowTitle(tr("Parameter graphs"));
    setWindowIcon(QIcon(":/grapher.png"));

    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);

    restore_geometry();

    dotboxCursor = new QCursor(QBitmap(":/dotbox.png"));
    setMouseTracking(true);

    createGui();
}

void StandGrapher::setPortableDrawing(bool p)
{
    if (portable_drawing == p) return;
    portable_drawing = p;

    REDRAW_WINDOW
}

void StandGrapher::setModel(Stand_trace_model::Ptr model)
{
    // Setup event filter to receive only "Update" events
    Selection event_filter = model->events();
    event_filter.disableAll();
    event_filter.setEnabled(event_filter.itemLink("Update"), true);
    model_ =
            dynamic_pointer_cast<Stand_trace_model>(model->filter_events(event_filter));

    filterWindow->setModel(model_);
    REDRAW_GRAPH
}

void StandGrapher::setModel(Trace_model::Ptr model)
{
    setModel(dynamic_pointer_cast<Stand_trace_model>(model));
}

void StandGrapher::createGui()
{
    //--------------------------------------------------
    // Universal button creator for the navigation bars
    //--------------------------------------------------
    class ButtonCreator
    {
    public:
        static QPushButton * create(QWidget * parent, const QString & objName, const QIcon & icon,
            const QKeySequence & key, QString toolTip, QWidget * target, const char * slot)
        {
            QPushButton * button = new QPushButton(parent);
            button->setObjectName(objName);
            button->setFocusPolicy(Qt::NoFocus);
            connect(button, SIGNAL(pressed()), target, slot);

            button->setIconSize(QSize(32, 32));
            button->setIcon(QIcon(icon));

            if (!key.isEmpty())
                toolTip += " " + tr("(Shortcut: <b>%1</b>)").arg(key.toString());
            button->setToolTip(toolTip);

            QShortcut * shortcut = new QShortcut(key, target);
            connect(shortcut, SIGNAL(activated()), target, slot);

            return button;
        }
    };

    QList<QPushButton *> buttons;

    //-------------------------------------------
    // Horizontal tool box
    //-------------------------------------------

    hToolBox = new QFrame(this);
    hToolBox->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    hToolBox->setAutoFillBackground(true);
    hToolBox->setCursor(Qt::ArrowCursor);
    hToolBox->installEventFilter(this);

    // Create buttons
    buttons << ButtonCreator::create(hToolBox, "Start_button", QIcon(":/start.png"),
        Qt::Key_Home, tr("Start"), this, SLOT(slotStart()));
    buttons << ButtonCreator::create(hToolBox, "Prev_button", QIcon(":/previous.png"),
        Qt::Key_Left, tr("Previous"), this, SLOT(slotPrevious()));
    buttons << ButtonCreator::create(hToolBox, "HFit_button", QIcon(":/fit.png"),
        QKeySequence(), tr("Fit horizontally"), this, SLOT(slotFit()));
    buttons << ButtonCreator::create(hToolBox, "Next_button", QIcon(":/next.png"),
        Qt::Key_Right, tr("Next"), this, SLOT(slotNext()));
    buttons << ButtonCreator::create(hToolBox, "Finish_button", QIcon(":/finish.png"),
        Qt::Key_End, tr("End"), this, SLOT(slotEnd()));
    buttons << ButtonCreator::create(hToolBox, "HZoomIn_button", QIcon(":/plus.png"),
        Qt::Key_Plus, tr("Wider"), this, SLOT(slotZoomIn()));
    buttons << ButtonCreator::create(hToolBox, "HZoomOut_button", QIcon(":/minus.png"),
        Qt::Key_Minus, tr("Narrower"), this, SLOT(slotZoomOut()));

    // Layout buttons
    QHBoxLayout * hLayout = new QHBoxLayout(hToolBox);
    foreach(QPushButton * button, buttons)
        hLayout->addWidget(button);
    hToolBox->setLayout(hLayout);

    //-------------------------------------------
    // Vertical tool box
    //-------------------------------------------

    vToolBox = new QFrame(this);
    vToolBox->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    vToolBox->setAutoFillBackground(true);
    vToolBox->setCursor(Qt::ArrowCursor);
    vToolBox->installEventFilter(this);

    buttons.clear();
    buttons << ButtonCreator::create(vToolBox, "Up_button", QIcon(":/up.png"),
        Qt::Key_Up, tr("Up"), this, SLOT(slotUp()));
    buttons << ButtonCreator::create(vToolBox, "VFit_button", QIcon(":/fit_vert.png"),
        QKeySequence(), tr("Fit vertically"), this, SLOT(slotVFit()));
    buttons << ButtonCreator::create(vToolBox, "Down_button", QIcon(":/down.png"),
        Qt::Key_Down, tr("Down"), this, SLOT(slotDown()));
    buttons << ButtonCreator::create(vToolBox, "VZoomIn_button", QIcon(":/plus.png"),
        Qt::SHIFT+Qt::Key_Plus, tr("Wider"), this, SLOT(slotVZoomIn()));
    buttons << ButtonCreator::create(vToolBox, "VZoomOut_button", QIcon(":/minus.png"),
        Qt::SHIFT+Qt::Key_Minus, tr("Narrower"), this, SLOT(slotVZoomOut()));

    // Layout buttons
    QVBoxLayout * vLayout = new QVBoxLayout(vToolBox);
    foreach(QPushButton * button, buttons)
        vLayout->addWidget(button);
    vToolBox->setLayout(vLayout);

    //-------------------------------------------
    // Tool Box
    //-------------------------------------------

    toolBox = new QFrame(this);
    toolBox->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    toolBox->setAutoFillBackground(true);
    toolBox->setCursor(Qt::ArrowCursor);
    toolBox->installEventFilter(this);

    buttons.clear();

    // Filter button
    filterButton = new QPushButton(toolBox);
    filterButton->setIcon(QIcon(":/filter.png"));
    filterButton->setCheckable(true);
    filterButton->setShortcut(Qt::Key_F);
    filterButton->setToolTip(tr("Filter window"));
    connect(filterButton, SIGNAL(released()), this, SLOT(slotFilterWindow()));
    buttons << filterButton;

    filterWindow = new ParamsFilterWidget(this);

    connect(filterWindow, SIGNAL( modelChanged(Stand_trace_model::Ptr) ),
        this, SLOT( setModel(Stand_trace_model::Ptr) ));

    connect(filterWindow, SIGNAL(windowClosed()),
        filterButton, SLOT(toggle()));

    // Drawing lines option
    drawLinesButton = new QPushButton(toolBox);
    drawLinesButton->setIcon(QIcon(":/draw_lines.png"));
    drawLinesButton->setCheckable(true);
    drawLinesButton->setChecked(true);
    drawLinesButton->setShortcut(Qt::Key_L);
    drawLinesButton->setToolTip(tr("Draw lines"));
    connect(drawLinesButton, SIGNAL(released()), this, SLOT(slotDrawLineSwither()));
    buttons << drawLinesButton;

    // Antialiasing option
    antialiasingButton = new QPushButton(toolBox);
    antialiasingButton->setIcon(QIcon(":/antialiasing.png"));
    antialiasingButton->setCheckable(true);
    antialiasingButton->setChecked(antialiasing);
    antialiasingButton->setShortcut(Qt::Key_A);
    antialiasingButton->setToolTip(tr("Antialiasing"));
    connect(antialiasingButton, SIGNAL(clicked(bool)), this, SLOT(slotAntialiasingSwither(bool)));
    buttons << antialiasingButton;

    // Print button
    printButton = new QPushButton(toolBox);
    printButton->setIcon(QIcon(":/printer.png"));
    printButton->setShortcut(Qt::Key_P);
    printButton->setToolTip(tr("Print"));
    connect(printButton, SIGNAL(released()), this, SLOT(slotPrintGraph()));
    buttons << printButton;

    // Setup buttons
    foreach (QPushButton * button, buttons) {
        button->setIconSize(QSize(16, 16));
        button->setFocusPolicy(Qt::NoFocus);
        button->setToolTip(button->toolTip() + " " +
            tr("(Shortcut: <b>%1</b>)").arg(button->shortcut().toString()));
    }

    // Layout buttons
    QGridLayout * tbLayout = new QGridLayout(toolBox);
    tbLayout->addWidget(filterButton, 0, 0);
    tbLayout->addWidget(drawLinesButton, 1, 0);
    tbLayout->addWidget(antialiasingButton, 1, 1);
    tbLayout->addWidget(printButton, 0, 1);
    toolBox->setLayout(tbLayout);

}

void StandGrapher::checkActions()
{
    Trace_model::Ptr m = model_;
    Trace_model::Ptr r = model_->root();

    bool min_time = m->min_time() == r->min_time();
    hToolBox->findChild<QPushButton *>("Start_button")
        ->setEnabled(!min_time);
    hToolBox->findChild<QPushButton *>("Prev_button")
        ->setEnabled(!min_time);

    bool max_time = m->max_time() == r->max_time();
    hToolBox->findChild<QPushButton *>("Finish_button")
        ->setEnabled(!max_time);
    hToolBox->findChild<QPushButton *>("Next_button")
        ->setEnabled(!max_time);

    bool max_zoom = (m->max_time()-m->min_time())/2 < m->min_resolution();
    hToolBox->findChild<QPushButton *>("HZoomIn_button")
        ->setEnabled(!max_zoom);

    bool full_trace = min_time && max_time;
    hToolBox->findChild<QPushButton *>("HFit_button")
        ->setEnabled(!full_trace);
    hToolBox->findChild<QPushButton *>("HZoomOut_button")
        ->setEnabled(!full_trace);

    double delta = (max_value - min_value)/4;
    bool can_down = max_value-delta > min_value_for_fit;
    vToolBox->findChild<QPushButton *>("Down_button")
        ->setEnabled(can_down);
    bool can_up = min_value+delta <= max_value_for_fit;
    vToolBox->findChild<QPushButton *>("Up_button")
        ->setEnabled(can_up);

    vToolBox->findChild<QPushButton *>("VFit_button")
        ->setEnabled(!alreadyFitted && !isinf(min_value_for_fit)
            && !isinf(max_value_for_fit));
}

void StandGrapher::moveToolBar(QFrame * toolbar, const QPoint & movement)
{
    QPoint newpos = toolbar->pos()+movement;

    if (newpos.x() < 0) newpos.setX(0);
    if (newpos.x()+toolbar->width() > width()) newpos.setX(width()-toolbar->width());

    if (newpos.y() < 1) newpos.setY(1);
    if (newpos.y()+toolbar->height() > height()) newpos.setY(height()-toolbar->height());

    toolbar->move(newpos);
}


//FIXME duplicated code, need to create class for preparing settings
void StandGrapher::prepare_settings(QSettings& settings,
                        const Trace_model::Ptr & model) const
{
Stand_trace_model* sm = dynamic_cast<Stand_trace_model*>(model.get());
    QString id = canonicalTraceId(sm->getPath() + ".trc");
if (!id.isEmpty())
    {
        settings.beginGroup(id);
    }
// FIXME: throw if 'id' is empty.
}

QString StandGrapher::canonicalTraceId(const QString& trc_file_path) const
{
    QString cp = QFileInfo(trc_file_path).canonicalFilePath();
    if (!cp.isEmpty())
    {
        // QSettigns has a bug whereas it's fine to have
        // key
        //   home\whatever\foo\bar
        // but QSetting::childKeys() will think 'home' is a group name
        // and won't return anything. So, pack the filename in
        // base64 string.
        QByteArray base64 = cp.toUtf8().toBase64();
        // But base64 uses this evil '/' character too. Fortunately
        // it does not use '*', so we replace.
        return QString(base64).replace('/', '*');
    }
    else
        return QString();
}

void StandGrapher::save_scope() const
{
    QSettings settings;
    prepare_settings(settings, model_);
    settings.setValue("grapher_min_value", min_value);
    settings.setValue("grapher_max_value", max_value);
    settings.setValue("grapher_min_time", model_->min_time().toString());
    settings.setValue("grapher_max_time", model_->max_time().toString());
    common::Selection params = model_->params();
    QList<int> queue;
    queue << Selection::ROOT;
    while (! queue.isEmpty()){
        int current = queue.takeFirst();
        if (params.hasChildren(current)){
            queue +=  params.items(current);
        }
        else{
            QString str = QString("grapher_color_")+QString::number(current);
            settings.setValue(str,paramColor(current));
        }
    }
}

void StandGrapher::restore_scope()
{
    QSettings settings;
    prepare_settings(settings, model_);
    double restored_max, restored_min;
    restored_min = settings.value("grapher_min_value").toDouble();
    restored_max = settings.value("grapher_max_value").toDouble();
    if (restored_max != restored_min){
        min_value = restored_min;
        max_value = restored_max;
    }
    QString min_time_str, max_time_str;
    common::Time min_time (model_->root()->min_time());
    common::Time max_time (model_->root()->max_time());
    min_time_str = settings.value("grapher_min_time").toString();
    max_time_str = settings.value("grapher_max_time").toString();
    if (max_time_str != min_time_str){
        min_time = min_time.fromString(min_time_str);
        max_time = max_time.fromString(max_time_str);
        setModel(model_->set_range(min_time, max_time));
    }
    common::Selection params = model_->params();
    QList<int> queue;
    queue << Selection::ROOT;
    while (! queue.isEmpty()){
        int current = queue.takeFirst();
        if (params.hasChildren(current)){
            queue +=  params.items(current);
        }
        else{
            QString str = QString("grapher_color_")+QString::number(current);
            QColor def = paramColor(current);
            QColor color = settings.value(str, def).value<QColor>();
            model_->setParamProperty(current, "color", color.rgb());
        }
    }
}

void StandGrapher::save_geometry() const
{
    QSettings settings;
    settings.setValue("graph_window_geometry", geometry());
}

void StandGrapher::restore_geometry()
{
    QSettings settings;
    QRect r = settings.value("graph_window_geometry").toRect();

    if (r.isValid())
        setGeometry(r);
    else {
        r = QApplication::desktop()->screenGeometry();
        resize(r.width()*80/100, r.height()*80/100);
    }
}

//---------------------------------------------------------------------------------------
// Events handling
//---------------------------------------------------------------------------------------

void StandGrapher::showEvent(QShowEvent *)
{
    if (!initialized) {
        hToolBox->move(axis_y_margin + (width()-axis_y_margin - hToolBox->width())/2, 1);
        vToolBox->move(width() - vToolBox->width(), (height()-axis_x_margin - vToolBox->height())/2);
        toolBox->move(width() - toolBox->width(), 1);

        initialized = true;
    }
}

void StandGrapher::closeEvent(QCloseEvent *)
{
    save_geometry();
    save_scope();

    // Stop background drawing
    drawing_needed = true;
}

void StandGrapher::resizeEvent(QResizeEvent * event)
{
    int oldw = event->oldSize().width();
    int oldh = event->oldSize().height();

    int w = event->size().width();
    int h = event->size().height();

    double ww = 1.0*w/oldw, hh = 1.0*h/oldh;

    QFrame * tb[] = { hToolBox, vToolBox, toolBox };
    for (int i = 0; i < 3; i++) {
        int x, y;

        // Calculate new X coordinate
        int tb_x = tb[i]->pos().x();
        int tb_w = tb[i]->width();

        if (tb_x <= 1)
            x = 0;
        else if (tb_x+tb_w >= oldw)
            x = w - tb_w;
        else {
            x = (int)((tb_x + tb_w/2)*ww) - tb_w/2;
            if (x < 0) x = 0;
            if (x + tb_w > w) x = w - tb_w;
        }

        // Calculate new Y coordinate
        int tb_y = tb[i]->pos().y();
        int tb_h = tb[i]->height();

        if (tb_y <= 1)
            y = 1;
        else if (tb_y+tb_h >= oldh)
            y = h - tb_h;
        else {
            y = (int)((tb_y + tb_h/2)*hh) - tb_h/2;
            if (y < 1) y = 1;
            if (y + tb_h > h) y = h - tb_h;
        }

        // Move toolbar to the new position
        tb[i]->move(x, y);
    }

    REDRAW_WINDOW
}

bool StandGrapher::eventFilter(QObject * obj, QEvent * event)
{
    static QPoint stored_pos;
    if (obj == hToolBox || obj == vToolBox || obj == toolBox) {

        if (event->type() == QEvent::MouseButtonPress) {

            QMouseEvent * e = static_cast<QMouseEvent*>(event);
            if (e->button() != Qt::LeftButton) return false;

            stored_pos = e->pos();
            return true;

        } else if (event->type() == QEvent::MouseMove) {

            QMouseEvent * e = static_cast<QMouseEvent*>(event);
            if (e->buttons() & Qt::LeftButton == 0) return false;

            QFrame * toolbar = static_cast<QFrame*>(obj);
            moveToolBar(toolbar, e->pos()-stored_pos);
            return true;

        }
    }

    return QDialog::eventFilter(obj, event);
}

//---------------------------------------------------------------------------------------
// Painting support
//---------------------------------------------------------------------------------------

int StandGrapher::pixelPositionForTime(const Time& time) const
{
    Time delta = time - model_->min_time();
    double ratio = delta/(model_->max_time()-model_->min_time());

    return int(ratio*area_width);
}

Time StandGrapher::timeForPixel(int pixel_x) const
{
    return Time::scale(model_->min_time(), model_->max_time(),
                       double(pixel_x)/area_width);
}

QColor StandGrapher::paramColor(int param) const
{
    return model_->params().itemProperty(param, "color").toUInt();
}

void StandGrapher::createCanvas()
{
    area_width = width() - axis_y_margin;
    area_height = height() - axis_x_margin;

    if (!canvas_.get())
        if (portable_drawing)
            canvas_.reset(new QImage(width(), height(), QImage::Format_RGB32));
        else
            canvas_.reset(new QPixmap(width(), height()));
}

void StandGrapher::needRedraw(bool recreateCanvas)
{
    if (recreateCanvas) canvas_.reset();

    drawing_needed = true; alreadyFitted = false;
    if (paramWnd) { delete paramWnd; paramWnd = 0; }
    update();
}

void StandGrapher::paintEvent(QPaintEvent *event)
{
    if (!model_.get()) return;

    // Start background drawing
    if (drawing_needed && !drawing_in_progress)
        QTimer::singleShot(0, this, SLOT( drawGraph() ));

    if (!canvas_.get()) return;

    QPainter painter(this);
    if (portable_drawing) {
        QImage * image = static_cast<QImage*>(canvas_.get());
        painter.drawImage(0, 0, *image);
    } else {
        QPixmap * pixmap = static_cast<QPixmap*>(canvas_.get());
        painter.drawPixmap(0, 0, *pixmap);
    }

    checkActions();
}

void StandGrapher::drawGraph(QPainter * p)
{
    if (drawing_in_progress) return;

    bool printer = (p != 0);
    if (!printer) {
        drawing_in_progress = true;
        drawing_needed = false;

        createCanvas();
        p = new QPainter(canvas_.get());
    }

    int width = area_width + axis_y_margin;
    int height = area_height + axis_x_margin;

    p->fillRect(0, 0, width, height, QBrush(Qt::white));

    // Drawing axises
    QBrush bgBrush = palette().brush(backgroundRole());
    p->fillRect(0, 0, axis_y_margin, height, bgBrush);
    p->fillRect(axis_y_margin, area_height, area_width, axis_x_margin, bgBrush);

    drawAxis(p, Qt::Horizontal);
    drawAxis(p, Qt::Vertical);

    // Create matrix for affine coordinate transformations
    // The center of coordinates is set to the cross point of
    // coordinate axis. Ox is directed to the right and Oy - to
    // the top of window.
    // These formulas describes the transformations:
    //     x' = 1*x + 0*y + axis_y_margin
    //     y' = 0*y + (-1)*y + area_height
    QMatrix m(1, 0, 0, -1, axis_y_margin, area_height);
    p->setWorldMatrix(m, true);

    p->setClipRect(0, 0, area_width-1, area_height-1);
    if (antialiasing) p->setRenderHint(QPainter::Antialiasing);

    // Setup values for vertical fitting
    min_value_for_fit = +INFINITY;
    max_value_for_fit = -INFINITY;

    QMap<int, QPoint> previousPoints;

    if  (!printer) {
        pointsHash.clear();

        painter_timer = startTimer(painter_timer_interval);
        painter_timer_supervisor.restart();
    }

    bool drawLines = drawLinesButton->isChecked();
    QSet<int> drawnedPoints;
    model_->rewind();
    for (;;) {
        // Obtain next parameter
        std::auto_ptr<Event_model> event(model_->next_event());
        if (!event.get()) break;

        Stand_update_event * uEv = static_cast<Stand_update_event*>(event.release());
        if (uEv->param.id == -1) continue;

        // Convert parameter's data to coordinates of a point
        int paramId = uEv->param.id;
        double paramValue = uEv->param.value.toDouble();

        QColor color = paramColor(uEv->param.id);
        int x = pixelPositionForTime(uEv->time);
        long long y = (long long)(area_height * (paramValue-min_value)/(max_value-min_value));
        if (y > INT_MAX) y = INT_MAX; if (y < INT_MIN) y = INT_MIN;

        // Draw point
        p->setPen(color);
        p->setBrush(color);

        if (y >= 0 && y < area_height && !drawnedPoints.contains(y*area_width+x)) {
            p->drawEllipse(x-2, y-2, 4, 4);
            drawnedPoints.insert(y*area_width+x);
        }

        // Draw line
        if (drawLines) {
            if (!previousPoints.contains(paramId)) {
                p->setPen(QPen(color, 1, Qt::DashLine));
                previousPoints[paramId] = QPoint(0, y);
            }

            p->drawLine(previousPoints[paramId], QPoint(x, y));
        }
        previousPoints[paramId] = QPoint(x, y);

        // Store min and max y coordinate values
        if (paramValue > max_value_for_fit)
            max_value_for_fit = paramValue;
        if (paramValue < min_value_for_fit)
            min_value_for_fit = paramValue;

        if (!printer) {
            // Store point to points hash
            pointsHash.insert(hash(x, y),
                shared_ptr<Stand_update_event>(uEv));

            // Process events for show current result.
            QApplication::processEvents();
            if (drawing_needed) { /* cancel current drawing */
                delete p; killTimer(painter_timer);
                drawing_in_progress = false;
                update(); return;
            }
        }
    }

    // Draw dotted continuation for graph.
    foreach (int paramId,  previousPoints.keys()) {
        QColor color = paramColor(paramId);
        p->setPen(QPen(color, 1, Qt::DashLine));
        p->setBrush(color);
        p->drawLine(previousPoints[paramId], QPoint(area_width, previousPoints[paramId].y()));
    }

    if  (!printer) {
        delete p; killTimer(painter_timer);
        repaint();
        drawing_in_progress = false;
    }
}

void StandGrapher::drawAxis(QPainter * p, Qt::Orientation orientation)
{
    p->save();

    // Draw line and marks
    int width; QMatrix m;
    if (orientation == Qt::Horizontal) {
        width = area_width;
        m.translate(axis_y_margin, area_height);
    } else {
        width = area_height;
        m.setMatrix(0, -1, -1, 0, axis_y_margin, area_height);
    }

    p->setWorldMatrix(m, true);
    p->setBrush(Qt::black);
    p->setPen(Qt::black);

    p->drawLine(0, 0, width-1, 0);
    p->setRenderHint(QPainter::Antialiasing);
    for(int pos = 0; pos < width; pos += 5){

        if (pos % 100 == 0) {
            QPolygon arrow(3);
            arrow.setPoint(0, pos, 2+2);
            arrow.setPoint(1, pos+3, 9+2);
            arrow.setPoint(2, pos-3, 9+2);
            p->drawPolygon(arrow);
        } else if (pos % 10 == 0) {
            p->drawLine(pos, 3+2, pos, 6+2);
        }

    }

    p->restore();

    // Draw labels
    QFontMetrics fm(p->font());
    QString label;
    if (orientation == Qt::Horizontal)
        for (int pos = 0; pos < width; pos += 100) {
            Time time_here = timeForPixel(pos);
            label = time_here.toString();
            int label_width = fm.width(label);

            int x = pos-label_width/2;
            if (x < -3) x = -3;
            if (x+label_width > width) x = width-label_width;

            p->drawText( x+axis_y_margin, area_height+12, label_width, 100,
                            Qt::AlignTop | Qt::AlignLeft, label);
        }
    else {
        p->save();
        p->setWorldMatrix(QMatrix(0, -1, 1, 0, axis_y_margin-10, area_height), true);

        // Calc count of numerals after decimal point
        int num_count = 0;
        double delta = (max_value-min_value)/(width/100);
        while ((int)delta == 0) {
            delta *= 10.0;
            num_count++;
        }

        // Draw labels for vertical axis
        int previous_end = -axis_x_margin-5;
        for (int pos = 0; pos < width; pos += 100) {
            double current = (max_value-min_value)*pos/width + min_value;
            label = QString::number(current, 'f', num_count);

            int label_width = fm.width(label);
            int label_height = fm.height();

            int x = pos-label_width/2;
            if (x < -axis_x_margin+3) x = -axis_x_margin+3;
            save_scope();
            if (x+label_width > width) x = width-label_width;

            if (x < previous_end) continue;
            previous_end = x + label_width;

            p->drawText(x, -label_height, label_width, 100,
                            Qt::AlignTop | Qt::AlignLeft, label);
        }

        p->restore();
    }
}

//---------------------------------------------------------------------------------------
// Parameter's info support
//---------------------------------------------------------------------------------------

uint StandGrapher::hash(int x, int y)
{
    double hash = floor(x / hashCellSize) +
        floor(y / hashCellSize) * ceil(area_width / hashCellSize);

    return (uint)hash;
}

uint StandGrapher::hash(const QPoint & p)
{
    return hash(p.x()-axis_y_margin, area_height-p.y());
}

void StandGrapher::pointsAbout(const QPoint & p, Stand_update_event_list & targetList)
{
    // Retrieve points from hash
    targetList.clear();
    foreach (shared_ptr<Stand_update_event> event, pointsHash.values(hash(p)))
    {
        // Leave only close points
        int X = p.x() - axis_y_margin;
        int Y = area_height - p.y();
        int mouseLookWidth2 = mouseLookWidth*mouseLookWidth;

        int x = pixelPositionForTime(event->time);
        int y = (int)(area_height * (event->param.value.toDouble()-min_value)/(max_value-min_value));

        if ((X-x)*(X-x)+(Y-y)*(Y-y) > mouseLookWidth2)
            continue;

        targetList << event.get();
    }
}

void StandGrapher::mouseMoveEvent(QMouseEvent * event)
{

// Cursors

    if ((event->pos().x() < axis_y_margin) || (event->pos().y() > area_height) ||
        (childAt(event->pos()) != 0)) {

        setCursor(Qt::ArrowCursor);
        return;
    }

    setCursor(*dotboxCursor);

// Tooltips

    Stand_update_event_list paramList;
    pointsAbout(event->pos(), paramList);

    if (paramList.isEmpty()) {
        QToolTip::hideText();
        return;
    }

    QString toolTip;

    if (paramList.isEmpty()) return;
    if (paramList.size() == 1) {
        Stand_update_event * event = paramList[0];

        QString formatStr("<b>%1:</b> %2");
        toolTip  = formatStr.arg(tr("Component")).arg(event->component_name()) + "<br>";
        toolTip += formatStr.arg(tr("Name")).arg(event->param_name()) + "<br>";
        toolTip += formatStr.arg(tr("Time")).arg(event->time.toString()) + "<br>";
        toolTip += formatStr.arg(tr("Value")).arg(event->param.value.toString());


    } else {
        for (int i = 0; i < 3; i++) {
            if (i >= paramList.size()) break;

            Stand_update_event * event = paramList[i];
            toolTip += event->component_name() + ", ";
            toolTip += event->param_name()+ ", "+event->time.toString()+", ";
            toolTip += event->param.value.toString();

            if (i < paramList.size()-1) toolTip += "<br>";
        } toolTip = "<b>"+toolTip+"</b>";

        if (paramList.size() > 3)
            toolTip  += "<br>"+tr("There are other points. Click to see them.");
    }

    QToolTip::showText(event->globalPos(), toolTip);
}

void StandGrapher::mousePressEvent(QMouseEvent * event)
{
    if ((event->pos().x() < axis_y_margin) || (event->pos().y() > area_height) ||
        (childAt(event->pos()) != 0)) {

        // Handle toolbox'es show/hide menu
        if (event->button() == Qt::RightButton) {
            QMenu * menu = new QMenu(this);
            menu->setAttribute(Qt::WA_DeleteOnClose, true);

            QAction * hPanelShowHide = new QAction(tr("Horizontal panel"), menu);
            hPanelShowHide->setCheckable(true);
            hPanelShowHide->setChecked(hToolBox->isVisible());
            connect(hPanelShowHide, SIGNAL( toggled(bool) ),
                hToolBox, SLOT(setVisible(bool)));

            QAction * vPanelShowHide = new QAction(tr("Vertical panel"), menu);
            vPanelShowHide->setCheckable(true);
            vPanelShowHide->setChecked(vToolBox->isVisible());
            connect(vPanelShowHide, SIGNAL( toggled(bool) ),
                vToolBox, SLOT(setVisible(bool)));

            QAction * tPanelShowHide = new QAction(tr("Tool panel"), menu);
            tPanelShowHide->setCheckable(true);
            tPanelShowHide->setChecked(toolBox->isVisible());
            connect(tPanelShowHide, SIGNAL( toggled(bool) ),
                toolBox, SLOT(setVisible(bool)));

            menu->addAction(hPanelShowHide);
            menu->addAction(vPanelShowHide);
            menu->addAction(tPanelShowHide);

            menu->popup(event->globalPos());
            event->accept(); return;
        }

        // It's not our event, let's the system examine one by itself
        QDialog::mousePressEvent(event);
        return;
    } else event->accept();

    Stand_update_event_list nearList;
    pointsAbout(event->pos(), nearList);

    if (paramWnd)
        delete paramWnd;

    paramWnd = new ParamInfoPopup(this);
    paramWnd->show(nearList);
}

void StandGrapher::wheelEvent(QWheelEvent * event)
{
#ifndef _CONCISE_
    Time time = timeForPixel(event->x() - axis_y_margin);
    double value = min_value + (area_height-event->y())*(max_value-min_value)/area_height;

    Time min_time, max_time;
    Time h_delta_l = time - model_->min_time();
    Time h_delta_r = model_->max_time()- time;

    double v_delta_l = value - min_value;
    double v_delta_h = max_value - value;

    if (event->delta() > 0)
    {
        if (h_delta_l + h_delta_r <= model_->min_resolution()) return;

        min_time = time - h_delta_l/2;
        max_time = time + h_delta_r/2;

        min_value = value - v_delta_l/2;
        max_value = value + v_delta_h/2;
    }
    else
    {
        min_time = time - h_delta_l*2;
        max_time = time + h_delta_r*2;

        if (min_time < model_->root()->min_time())
            min_time = model_->root()->min_time();

        if (max_time > model_->root()->max_time())
            max_time = model_->root()->max_time();

        min_value = value - v_delta_l*2;
        max_value = value + v_delta_h*2;
    }

    setModel(model_->set_range(min_time, max_time));
    REDRAW_GRAPH
#endif
}

//---------------------------------------------------------------------------------------
// Slots for toolbar's actions
//---------------------------------------------------------------------------------------

void StandGrapher::slotStart()
{
    Time delta = model_->max_time() - model_->min_time();
    Time new_min = model_->root()->min_time();

    Time new_max = new_min + delta;
    if (new_min == model_->min_time()) return;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotPrevious()
{
    Time delta = model_->max_time() - model_->min_time();

    Time new_min = model_->min_time()-delta/4;
    Time root_min = model_->root()->min_time();
    if (new_min < root_min)
        new_min = root_min;

    Time new_max = new_min + delta;
    if (new_min == model_->min_time()) return;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotFit()
{
    Time new_min = model_->root()->min_time();
    Time new_max = model_->root()->max_time();

    if (model_->max_time()-model_->min_time() == new_max-new_min)
        return;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotNext()
{
    Time delta = model_->max_time() - model_->min_time();

    Time new_max = model_->max_time()+delta/4;
    if (new_max > model_->root()->max_time())
        new_max = model_->root()->max_time();

    Time new_min = new_max - delta;
    if (new_max == model_->max_time()) return;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotEnd()
{
    Time delta = model_->max_time() - model_->min_time();

    Time new_max = model_->root()->max_time();
    Time new_min = new_max - delta;

    if (new_max == model_->max_time()) return;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotZoomIn()
{
    Time delta = model_->max_time() - model_->min_time();
    if (delta < model_->min_resolution()) return;

    Time new_min = model_->min_time()+delta/4;
    Time new_max = model_->max_time()-delta/4;

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotZoomOut()
{
    Time delta = model_->max_time() - model_->min_time();

    Time new_min = model_->min_time()-delta/2;
    Time new_max = model_->max_time()+delta/2;

    if (new_min < model_->root()->min_time())
        new_min = model_->root()->min_time();

    if (new_max > model_->root()->max_time())
        new_max = model_->root()->max_time();

    setModel(model_->set_range(new_min, new_max));
}

void StandGrapher::slotUp()
{
    double delta = (max_value - min_value)/16;
    if (min_value+delta > max_value_for_fit) return;

    min_value += delta;
    max_value += delta;

    REDRAW_GRAPH
}

void StandGrapher::slotVFit()
{
    if (isinf(max_value_for_fit)) return;
    if (min_value_for_fit == max_value_for_fit) return;
    if (alreadyFitted) return;

    double margins = 0.01*(max_value_for_fit-min_value_for_fit);
    min_value = min_value_for_fit-margins;
    max_value = max_value_for_fit+margins;

    /* Enable up and down actions */
    vToolBox->findChild<QPushButton *>("Up_button")
        ->setEnabled(true);
    vToolBox->findChild<QPushButton *>("Down_button")
        ->setEnabled(true);

    REDRAW_GRAPH
    alreadyFitted = true;
}

void StandGrapher::slotDown()
{
    double delta = (max_value - min_value)/16;
    if (max_value-delta <= min_value_for_fit) return;

    min_value -= delta;
    max_value -= delta;

    REDRAW_GRAPH
}

void StandGrapher::slotVZoomIn()
{
    double delta = max_value - min_value;
    min_value += delta/4;
    max_value -= delta/4;

    REDRAW_GRAPH
}

void StandGrapher::slotVZoomOut()
{
    double delta = max_value - min_value;
    min_value -= delta/2;
    max_value += delta/2;

    REDRAW_GRAPH
}

void StandGrapher::slotFilterWindow()
{
    if (!filterButton->isChecked()) {
        filterWindow->close();
        filterButton->toggle();

        return;
    }

    filterWindow->show();
}

void StandGrapher::slotDrawLineSwither()
{
    REDRAW_GRAPH
}

void StandGrapher::slotAntialiasingSwither(bool state)
{
    antialiasing = state;

    REDRAW_GRAPH
}

void StandGrapher::slotPrintGraph()
{
    QPrinter printer;
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer, this->parentWidget());

    // Show dialog
    if (printDialog.exec() == QDialog::Accepted)
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // Set values of area_width and area_height to paper size
        area_width = printer.width() - axis_y_margin;
        area_height = printer.height() - axis_x_margin;

        // Printing
        QPainter p(&printer);
        drawGraph(&p);

        QApplication::restoreOverrideCursor();
    }

    REDRAW_WINDOW
}

void StandGrapher::timerEvent(QTimerEvent * event)
{
    // Try to find optimal timer inverval value.
    static int previous_result = 0;

    int real_interval = painter_timer_supervisor.elapsed();
    int current_result = real_interval - painter_timer_interval;

    if (current_result > previous_result)
        painter_timer_interval += 50;
    else
        painter_timer_interval -= 50;

    if (painter_timer_interval < 0)
        painter_timer_interval = 0;

    killTimer(painter_timer);
    painter_timer = startTimer(painter_timer_interval);
    painter_timer_supervisor.restart();

    previous_result = current_result;

    // Repaint canvas with current drawing result.
    repaint();
}

}

}
