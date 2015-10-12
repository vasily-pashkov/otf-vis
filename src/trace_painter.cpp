#include "trace_painter.h"
#include "trace_model.h"
#include "state_model.h"
#include "group_model.h"
#include "event_model.h"

#include <QPrinter>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QApplication>
#include <QSet>
#include <QSettings>

namespace {

unsigned joaat_hash (const unsigned char *key, size_t len)
{
     unsigned hash = 0;
     size_t i;

     for (i = 0; i < len; i++)
     {
         hash += key[i];
         hash += (hash << 10);
         hash ^= (hash >> 6);
     }
     hash += (hash << 3);
     hash ^= (hash >> 11);
     hash += (hash << 15);
     return hash;
}

/** Number of visible letters in component's labels */
const int component_name_length = 11;

/** Stores information about event letter we must
    draw, it's position and it's bounding rect. */
struct Event_letter_drawing
{
    char letter;
    char subletter;
    QPoint letterPosition;
    QPoint subletterPosition;
    QRect boundingRect;
    unsigned priority;
};

}

unsigned int qHash(const std::pair< std::pair<int, int>, std::pair<int, int> >&
                   p)
{
    return joaat_hash((const unsigned char*)(&(p.first.first)),
                      sizeof(int)*4);
}

namespace vis4 {

using std::vector;
using std::pair;

using common::Time;
using common::Selection;

Trace_painter::Trace_painter()
    : right_margin(5), painter(0), tg(0), state_(Ready)
{
    QFontMetrics fm(QApplication::font());
    text_elements_height = (fm.height() + 2)/2*2;
    text_height = fm.height();
    text_letter_width = fm.maxWidth();

    left_margin1 = (component_name_length+1)*fm.width("m") + 30;
    left_margin2 = text_height+10;
    left_margin = left_margin1;

    timeline_height = timeline_text_top+text_height;

    lifeline_stepping = text_elements_height*3 + text_elements_height/2
        + (event_line_extra_height + event_line_and_letter_spacing)*2;

    y_unparented =
        text_elements_height/2 // upper half of lifeline itself
        + text_elements_height // letters above it.
        + text_elements_height // time tooltip
        + event_line_extra_height
        + event_line_and_letter_spacing;


    // Load component label colors

    QSettings settings;
    settings.beginGroup("colors");

	if (!settings.contains("component_rchm"))
		settings.setValue("component_rchm", "#9CCEFF");
	if (!settings.contains("component_chm"))
		settings.setValue("component_chm", "#10E205");
	if (!settings.contains("component_channel"))
		settings.setValue("component_channel", "#7979F1");
	if (!settings.contains("component_interface"))
		settings.setValue("component_interface", "#EEEEEE");
	if (!settings.contains("component_externals"))
		settings.setValue("component_externals", "#F87C7C");

    componentLabelColors.insert(Trace_model::RCHM,
        settings.value("component_rchm").toString());
    componentLabelColors.insert(Trace_model::CHM,
        settings.value("component_chm").toString());
    componentLabelColors.insert(Trace_model::CHANNEL,
        settings.value("component_channel").toString());
    componentLabelColors.insert(Trace_model::INTERFACE,
        settings.value("component_interface").toString());
    componentLabelColors.insert(Trace_model::EXTERNAL_OBJECTS,
        settings.value("component_externals").toString());

}

Trace_painter::~Trace_painter()
{
    if (painter) delete painter;
}

#define NP if (!printer_flag)

void Trace_painter::setModel(Trace_model::Ptr & model_)
{
    Q_ASSERT(model_.get());
    model = model_;
}

void Trace_painter::setPaintDevice(QPaintDevice * paintDevice)
{
    Q_ASSERT(paintDevice);

    if (painter) delete painter;
    painter = new QPainter(paintDevice);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    printer_flag = (dynamic_cast<QPrinter*> (paintDevice) != 0);

    width = painter->device()->width();
    height = painter->device()->height();

    components_per_page = (height-timeline_height -
        (y_unparented-lifeline_stepping/2)) / lifeline_stepping;
}

std::auto_ptr<Trace_geometry> Trace_painter::traceGeometry() const
{
    return std::auto_ptr<Trace_geometry>(tg);
}

QRect Trace_painter::drawTextBox(
    const QString& text, QPainter * painter,
    int x, int y, int width, int height,
    int text_start_x)
{
    QFontMetrics fm(QApplication::font());
    if (painter != 0)
        fm = painter->fontMetrics();

    int left_right_pad = fm.width('i');

    if (width == -1)
    {
        width = fm.width(text);
        width += 2*left_right_pad;
    }

    QRect r(x, y-height/2, width, height);

    if (painter)
        painter->drawRect(r);

    QRect text_r(r);
    text_r.adjust(left_right_pad, 0, -left_right_pad, 0);

    if (text_start_x != -1)
    {
        text_r.setLeft(text_start_x);
    }

    if (painter)
        painter->drawText(text_r, Qt::AlignLeft|Qt::AlignVCenter, text);

    // Note: when calling with null QPainter to get just metrics,
    // we can't be sure that pen width will be 1 in subsequent calls.
    // Have to trust the user.
    int pen_width = 1;
    r.adjust(-pen_width, -pen_width, pen_width, pen_width);

    return r;
}

QRect Trace_painter::drawPageMarker(QPainter * painter,
                                    int page, int count,
                                    int x, int y, bool horiz)
{
    QFont font(QApplication::font());
    font.setPointSize(10);
    QFontMetrics fm(font);

    QString text = QString(" %1/%2 ").arg(page).arg(count);

    int th = fm.height();
    int tw = fm.width(text);

    if (painter) {
        painter->save();

        painter->setFont(font);
        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);

        QRect r(th/2, -th/2, tw, th);
        QPolygon arrow_1, arrow_2;
        arrow_1 << QPoint(0, 0) << QPoint(th/2, -th/2) << QPoint(th/2, th/2);
        arrow_2 << QPoint(tw+th, 0) << QPoint(tw+th/2, -th/2) << QPoint(tw+th/2, th/2);

        if (horiz) painter->translate(x, y);
        else {
            painter->rotate(360-90);
            painter->translate(-y-tw-th, x-th);
        }

        painter->drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextIncludeTrailingSpaces, text);
        painter->setBrush(Qt::black);
        painter->drawPolygon(arrow_1);
        painter->drawPolygon(arrow_2);

        painter->restore();
    }

    if (horiz) return QRect(x, y - th/2, tw + th, th);
    else       return QRect(x - th/2, y, th + 2*th, tw + th);
}

void Trace_painter::draw_unified_arrow(int x1, int y1, int x2, int y2, QPainter * painter,
                        bool always_straight, bool start_arrowhead)
{
  // The length of the from the tip of the arrow to the point
    // where line starts.
    const int arrowhead_length = 16;

    QPainterPath arrow;
    arrow.moveTo(x1, y1);

    // Determine the angle of the straight line.
    double a1 = (x2-x1);
    double a2 = (y2-y1);
    double b1 = 1;
    double b2 = 0;

    double straight_length = sqrt(a1*a1 + a2*a2);

    double dot_product = a1*b1 + a2*b2;
    double cosine = dot_product/
        (sqrt(pow(a1, 2) + pow(a2, 2))*sqrt(b1 + b2));
    double angle = acos(cosine);
    if (y1 < y2)
    {
        angle = -angle;
    }
    double straight_angle = angle*180/M_PI;

    double limit = 10;

    double angle_to_vertical;
    if (fabs(straight_angle) < 90)
        angle_to_vertical = fabs(straight_angle);
    else if (straight_angle > 0)
        angle_to_vertical = 180-straight_angle;
    else
        angle_to_vertical = 180-(-straight_angle);

    double angle_delta = 0;
    if (!always_straight)
        if (angle_to_vertical > limit)
            angle_delta = 30 * (angle_to_vertical - limit)/90;
    double start_angle = straight_angle > 0
        ? straight_angle - angle_delta :
        straight_angle + angle_delta;


    QMatrix m1;
    m1.translate(x1, y1);
    m1.rotate(-start_angle);

    double end_angle = straight_angle > 0
        ? (straight_angle + 180 + angle_delta) :
        (straight_angle + 180 - angle_delta);

    QMatrix m2;
    m2.reset();
    m2.translate(x2, y2);
    m2.rotate(-end_angle);

    arrow.cubicTo(m1.map(QPointF(straight_length/2, 0)),
                  m2.map(QPointF(straight_length/2, 0)),
                  m2.map(QPointF(arrowhead_length, 0)));

    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(arrow);
    painter->restore();


    QPolygon arrowhead(4);
    arrowhead.setPoint(0, 0, 0);
    arrowhead.setPoint(1, arrowhead_length/3, -arrowhead_length*5/4);
    arrowhead.setPoint(2, 0, -arrowhead_length);
    arrowhead.setPoint(3, -arrowhead_length/3, -arrowhead_length*5/4);

    painter->save();

    painter->translate(x2, y2);
    painter->rotate(-90);
    painter->rotate(-end_angle);
    painter->rotate(180);
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(arrowhead);

    painter->restore();

    if (start_arrowhead)
    {
        painter->save();
        painter->translate(x1, y1);

        painter->rotate(-90);
        painter->rotate(-end_angle);

        painter->drawPolygon(arrowhead);
        painter->restore();
    }
}

int Trace_painter::pixelPositionForTime(const Time& time) const
{
    int lifelines_width = width - left_margin - right_margin;

    Time delta = time - model->min_time();

    double ratio = delta/timePerPage;
    return int(ratio*lifelines_width + left_margin);
}

Time Trace_painter::timeForPixel(int pixel_x) const
{
    // If x coordinate less than timeline
    // start margin, returns minimum time.
    if (pixel_x < left_margin) return model->min_time();

    unsigned lifelines_width = width-left_margin-right_margin;
    pixel_x -= left_margin;

    Q_ASSERT(!timePerPage.isNull());
    return Time::scale(model->min_time(), model->min_time()+timePerPage,
                       double(pixel_x)/lifelines_width);
}

void Trace_painter::splitToPages()
{
    // Calculate the number of pages
    pages_horizontally = 1;
    if (model->max_time()-model->min_time() > timePerFirstPage)
        pages_horizontally += (int)ceil( (model->max_time()-model->min_time()-timePerFirstPage)/timePerFullPage );

    pages_vertically = (int)ceil( 1.0*model->visible_components().size() / components_per_page );

    //qDebug() << "Number of pages:" << pages_horizontally << "/" << pages_vertically;
}

void Trace_painter::drawPage(int i, int j)
{
    // Calculate the number of components by vertical
    int from_component = (j == 0) ? 0 : j*components_per_page;
    int to_component = from_component + components_per_page-1;

    if (to_component >= model->visible_components().size())
        to_component = model->visible_components().size()-1;

    timePerPage = (i > 0) ? timePerFullPage : timePerFirstPage;

    Time min_time = model->min_time();
    if (i > 0) min_time = min_time + timePerFirstPage + timePerFullPage*(i-1);

    Time max_time = min_time + timePerPage;
    if (max_time > model->max_time()) max_time = model->max_time();

    // FIXME: this temporary change of model is ugly.
    Trace_model::Ptr saved_model = model;
    model = model->set_range(min_time, max_time);

    if (i == 0) left_margin = left_margin1;
    else        left_margin = left_margin2;

    drawComponentsList(from_component, to_component, i == 0);
    QApplication::processEvents();
    if (state_ == Canceled) return;

    painter->setClipRect(left_margin, y_unparented-lifeline_stepping/2,
        width-right_margin-left_margin, components_per_page*lifeline_stepping);

    drawEvents(from_component, to_component);
    if (state_ == Canceled) return;

    drawStates(from_component, to_component);
    if (state_ == Canceled) return;

    drawGroups(from_component, to_component);
    if (state_ == Canceled) return;

    if (printer_flag) {

        // Draw timeline
        painter->setClipRect(0, 0, width, height);
        drawTimeline(painter, 0, height - (timeline_text_top+text_height));

        // Draw labels with page numbers
        int x, y; QRect r;

        x = 0; y = 0;
        r = drawPageMarker(0, i+1, pages_horizontally, x, y, true);
        x = width - r.width(); y = r.height()/2;
        drawPageMarker(painter, i+1, pages_horizontally, x, y, true);

        x = 0; y = 0;
        r = drawPageMarker(0, j+1, pages_vertically, x, y, false);
        x = r.width()/2; y = height - r.height();
        drawPageMarker(painter, j+1, pages_vertically, x, y, false);

    }

    model = saved_model;
}

#define DL if (drawLabels)

void Trace_painter::drawComponentsList(int from_component, int to_component, bool drawLabels)
{
NP  {
        QLinearGradient g(0, 0, left_margin, 0);
        g.setColorAt(0, QColor(150, 150, 150));
        g.setColorAt(1, Qt::white);
        painter->setBrush(g);
        painter->setPen(Qt::NoPen);
        painter->drawRect(0, 0, left_margin, height);
    }

    // Draw the component list
    painter->setPen(Qt::black);
    painter->setClipRect(0, 0, width-right_margin, height);

    unsigned component_start = 5;
    unsigned component_width = left_margin-30;
    unsigned y = y_unparented;

    int parent = model->parent_component();
    if (parent != Selection::ROOT && from_component == 0)
    {
        // For parent component, don't reserve too much space above it.
        y = text_elements_height;

DL      {
            // Set background color for current component type
            painter->setBrush(componentLabelColors[model->component_type(parent)]);

            QString parent_name = model->component_name(model->parent_component());
            QRect r = drawTextBox(parent_name, painter,
                                  component_start, y, component_width + 15,
                                 text_elements_height);

NP          tg->clickable_components.push_back(qMakePair(r, model->components().itemParent(parent)));
NP          tg->componentlabel_rects.push_back(qMakePair(r, parent));
        }

        // For parent component, we don't see too much space below it
        y = y_unparented;
    }

    // Calculate lifeline position for each component.
    // Components, that lay higer than first visible,
    // has negative coordinats. This is needed for
    // correct arrows drawing.

    lifeline_position.clear();
    y -= from_component*lifeline_stepping; component_start += 15;

    for(int i = 0; i < model->visible_components().size(); i++)
    {
        int component = model->visible_components()[i];

        if (i >= from_component && i <= to_component) {
            bool composite = model->has_children(component);
            QString name = model->component_name(component);

            // Set background color for current component type
            painter->setBrush(componentLabelColors[model->component_type(component)]);

DL          if (composite)
            {
                QRect r = drawTextBox(name, painter, component_start + 4, y + 4,
                            component_width, text_elements_height);

                r.translate(-4, -4);
NP              tg->clickable_components.push_back(qMakePair(r, component));
            }

DL          {
                QRect r = drawTextBox(name, painter, component_start, y,
                                      component_width, text_elements_height);
NP              tg->componentlabel_rects.push_back(qMakePair(r, component));
            }

NP          {
                QRect r = QRect(0, y - text_elements_height/2+1,
                                width-right_margin, text_elements_height+2);
                tg->lifeline_rects.push_back(qMakePair(r, component));
            }

            painter->drawLine(left_margin, y, width, y);
        }

        lifeline_position.push_back(y);
        y += lifeline_stepping;
    }
}

#undef DL

void Trace_painter::drawStates(int from_component, int to_component)
{
    model->rewind();

    for(;;)
    {
        std::auto_ptr<State_model> s = model->next_state();
        if (!s.get()) break;

        int lifeline = model->lifeline(s.get()->component);
        if (lifeline < from_component || lifeline > to_component) continue;

        int pixel_begin = pixelPositionForTime(s->begin);
        int pixel_end = pixelPositionForTime(s->end);

        painter->setBrush(s->color);

        int text_begin = -1;
        if (pixel_begin < left_margin)
        {
            pixel_begin = left_margin-10;
            text_begin = left_margin;
        }
        if (pixel_end > width-right_margin)
            pixel_end = width-right_margin+10;

        /* If a state takes only one pixel, prune it. */
        if (pixel_end != pixel_begin)
        {
            QRect r = drawTextBox(model->states().item(s->type), painter,
                                  pixel_begin, lifeline_position[lifeline],
                                  pixel_end-pixel_begin, text_elements_height,
                                  text_begin);

            if (!printer_flag)
            {
                State_model* sm = s.release();
                tg->states.push_back(
                    qMakePair(r, boost::shared_ptr<State_model>(sm)));
            }
        }

        QApplication::processEvents();
        if (state_ == Canceled) return;
    }
}

void Trace_painter::drawEvents(int from_component, int to_component)
{
    QFontMetrics mainFontMetrics(painter->font());

    int mainFontAscent = mainFontMetrics.ascent();
    int mainFontDescent = mainFontMetrics.descent();
    int mainFontHeight = mainFontMetrics.height();

    QFont smallFont(painter->font());
    smallFont.setPointSize(smallFont.pointSize()*70/100);
    QFontMetrics smallFontMetrics(smallFont);

    vector<int> mainFontLetterWidth(256);
    for(unsigned i = 0; i < 256; ++i)
    {
        char l = (char)i;
        mainFontLetterWidth[i] = mainFontMetrics.width(QChar(l));
    }

    vector<int> smallFontLetterWidth(256);
    for(unsigned i = 0; i < 256; ++i)
    {
        char l = (char)i;
        smallFontLetterWidth[i] = smallFontMetrics.width(QChar(l));
    }

    // To resolve overlapping letter by removing a latter
    // is less priority, we store all letters we want to draw in
    // list, and draw only after all events are processed.
    QVector <QList<Event_letter_drawing> > letters_to_draw;
    letters_to_draw.resize(model->visible_components().size());

    // With large scales, many events might want to the same pixel.
    // Drawing line for each is very slow -- because the line drawing
    // code in Qt is not trivial. Also, paining all trace in event lines
    // is ugly. So we use this vector to remember last position of
    // line and draw event line once every 3 pixels.
    vector<int> last_event_line(model->visible_components().size(), -10);

    model->rewind();
    for(;;)
    {
        std::auto_ptr<Event_model> e = model->next_event();
        if (!e.get()) break;

        int lifeline = model->lifeline(e.get()->component);
        if (lifeline < from_component || lifeline > to_component) continue;

        int pos = pixelPositionForTime(e->time);

        // Workaround a bug in tracedb -- it often
        // returns event outside the requested time
        // range.
        if (pos < 0 || pos >= width)
            continue;

        unsigned y = lifeline_position[lifeline];

        // This is optimization. Drawing a line is much
        // more expensive than comparing two integers and we don't
        // ever need to draw a line on top of an already
        // drawn one.
        bool was_drawned = false;
        if (pos > last_event_line[lifeline] + 2)
        {
            painter->save();
            painter->setPen(QPen(Qt::black, 2));
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->drawLine(pos, y-text_elements_height/2-
                              event_line_extra_height,
                             pos, y+text_elements_height/2
                              +event_line_extra_height);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->restore();
            last_event_line[lifeline] = pos;

            was_drawned = true;
        }

NP      tg->eventsNear[lifeline][pos] = true;

        int letter_width = mainFontLetterWidth[(unsigned char)(e->letter)];
        int subletter_width = e->subletter ?
            smallFontLetterWidth[(unsigned char)(e->subletter)] : 0;


        unsigned letter_x = pos;
        unsigned letter_y = y - text_elements_height/2
            - event_line_extra_height - event_line_and_letter_spacing;

        if (e->letter_position == Event_model::left_top
            || e->letter_position == Event_model::left_bottom)
        {
            letter_x = pos - letter_width - subletter_width - 1;
        }

        if (e->letter_position == Event_model::left_bottom
            || e->letter_position == Event_model::right_bottom)
        {
            letter_y = y+text_elements_height/2
                + event_line_extra_height + event_line_and_letter_spacing
                + mainFontAscent;
        }

        // Compute the bounding rect of this letter.
        // Note that instead of QFontMetrics::boundingRect we use
        // 'width', so the right boundary of rect will be the position
        // where the next letter can be drawn.
        QRect bound(letter_x, letter_y-mainFontDescent,
                    letter_width + subletter_width + 1, mainFontHeight);

        Event_letter_drawing drawing;
        drawing.priority = e->priority;
        drawing.letter = e->letter;
        drawing.letterPosition = QPoint(letter_x, letter_y);
        letter_x += letter_width;
        drawing.subletter = e->subletter;
        drawing.subletterPosition = QPoint(letter_x, letter_y);
        drawing.boundingRect = bound;

        // Now see if this letter overlaps with any previously drawn letters.
        // The letters are stored sorted by the right boundary.
        bool deleted = false;
        QList<Event_letter_drawing>::iterator le;
        le = letters_to_draw[lifeline].end();

        // Note: we can't cache 'begin()' here since
        // 'begin()' iterator does not appear to be
        // stable, at least when all elements gets erased.
        while(le != letters_to_draw[lifeline].begin())
        {
            --le;
            if (le->boundingRect.right() <= bound.left())
                break;

            if (!(le->boundingRect & bound).isEmpty())
            {
                // We've got intersection. Remove either this
                // event or the previous one.
                if (le->priority >= e->priority)
                {
                    deleted = true;
                }
                else
                {
                    le = letters_to_draw[lifeline].erase(le);
                }
            }
        }

        if (!deleted)
        {
            // Must insert new letter while maintaining 'sort by right border'
            // property.
            QList<Event_letter_drawing>::iterator lb
                = letters_to_draw[lifeline].begin();
            le = letters_to_draw[lifeline].end();
            while(le != lb)
            {
                --le;
                if (bound.right() >= le->boundingRect.right())
                {
                    ++le;
                    break;
                }
            }
            letters_to_draw[lifeline].insert(le, drawing);
        }

        if (!printer_flag && was_drawned) {
            QApplication::processEvents();
            if (state_ == Canceled) return;
        }
    }

    for (int i = 0; i < letters_to_draw.size(); ++i)
    {
        Event_letter_drawing d;
        foreach(d, letters_to_draw[i])
        {
            painter->drawText(d.letterPosition, QChar(d.letter));

            if (d.subletter)
            {
                painter->save();
                painter->setFont(smallFont);
                painter->drawText(d.subletterPosition, QChar(d.subletter));
                painter->restore();
            }
        }
    }
}

void Trace_painter::drawGroups(int from_comp, int to_comp)
{
    QColor groups_color(Qt::darkGreen);
    //groups_color.setAlpha(200);
    painter->setBrush(groups_color);
    painter->setPen(Qt::darkGreen);

    QSet< pair< pair<int, int>, pair<int, int> > > drawn;

    model->rewind();
    for(;;)
    {
        std::auto_ptr<Group_model> g = model->next_group();
        if (!g.get()) break;

        if (g->type == Group_model::arrow)
        {
            int from_lifeline = model->lifeline(g->points[0].component);
            int from_pixel = pixelPositionForTime(g->points[0].time);
            pair<int, int> from_p(from_lifeline, from_pixel/9);
            QPoint from(from_pixel, lifeline_position[from_lifeline]);

            for(unsigned i = 1; i < g->points.size(); ++i)
            {
                int to_lifeline = model->lifeline(g->points[i].component);

                // For composite lifelines, both endpoints of an
                // error can end up on the same visible lifeline.
                // Nothing should be drawn in this case.
                if (to_lifeline == from_lifeline)
                    continue;

                // Don't try to draw invisible arrow
                if (((from_lifeline < (int)from_comp) || (from_lifeline > (int)to_comp)) &&
                    ((to_lifeline < (int)from_comp) || (to_lifeline > (int)to_comp))) continue;

                int to_pixel = pixelPositionForTime(g->points[i].time);
                pair<int, int> to_p(to_lifeline, to_pixel/9);

                // See we we've drawn an arrow between those endpoints already.
                pair< pair<int, int>, pair<int, int> > probe(from_p, to_p);
                if (!drawn.contains(probe))
                {
                    drawn.insert(probe);

                    QPoint to(to_pixel, lifeline_position[to_lifeline]);

                    int delta = text_elements_height/2+7;
                    QPoint fromAdjusted = from;
                    if (fromAdjusted.y() < to.y())
                    {
                        fromAdjusted.setY(fromAdjusted.y() + delta);
                        to.setY(to.y() - delta);
                    }
                    else
                    {
                        fromAdjusted.setY(fromAdjusted.y() - delta);
                        to.setY(to.y() + delta);
                    }
                    draw_unified_arrow(fromAdjusted.x(), fromAdjusted.y(),
                                       to.x(), to.y(), painter);
                }
            }
        }

        if (!printer_flag) {
            QApplication::processEvents();
            if (state_ == Canceled) return;
        }
    }
}

void Trace_painter::drawTrace(const Time & timePerPage, bool start_in_background)
{
    Q_ASSERT(painter);
    Q_ASSERT(model.get());

    // Special case when all components are filtered
    if (model->visible_components().size() == 0)
    {
        painter->fillRect(0, 0, width, height, Qt::white);

        QLinearGradient g(0, 0, left_margin, 0);
        g.setColorAt(0, QColor(150, 150, 150));
        g.setColorAt(1, Qt::white);
        painter->setBrush(g);
        painter->setPen(Qt::NoPen);
        painter->drawRect(0, 0, left_margin, height);
        return;
    }

    state_ = (start_in_background) ? Background : Active;

    this->timePerPage = timePerPage;
    timePerFirstPage = timePerPage;
    timePerFullPage = timePerFirstPage *
        (width-left_margin2-right_margin) / (width-left_margin1-right_margin);

    // Draw trace  on the printer...
    if (printer_flag)  {
        QPrinter * printer = dynamic_cast<QPrinter*>(painter->device());
        splitToPages();

        painter->save();
        for (int i = 0; i < pages_horizontally; i++)
            for (int j = 0; j < pages_vertically; j++) {
                if ((i > 0) || (j > 0)) printer->newPage();
                drawPage(i, j);
            }
        painter->restore();

        if (state_ != Canceled) state_ = Ready;
        return;
    }

    // ...or on the screen

    if (!tg) tg = new Trace_geometry();

    tg->clickable_components.clear();
    tg->states.clear();
    tg->lifeline_rects.clear();
    tg->componentlabel_rects.clear();

    tg->eventsNear.clear();
    tg->eventsNear.resize(model->visible_components().size());
    for(int i = 0; i < model->visible_components().size(); ++i)
    {
        tg->eventsNear[i].resize(width);
    }

    painter->fillRect(0, 0, width, height, Qt::white);
    painter->save(); drawPage(0, 0); painter->restore();
    if (state_ != Canceled) state_ = Ready;
}

void Trace_painter::drawTimeline(QPainter * painter, int x, int y)
{
    if (!model) return;

    painter->save();
    painter->translate(x, y);

    int width = painter->device()->width();
    QRect geometry(0, 0, width, timeline_height);
    painter->fillRect(geometry, QColor(255, 255, 255, 100));

    painter->setBrush(Qt::black);
    painter->setPen(Qt::black);

    painter->drawLine(left_margin, 0, width-1, 0);
    painter->setRenderHint(QPainter::Antialiasing);

    unsigned pixel_lenth = width-right_margin-left_margin;

    int max = width-right_margin;
    for(int pos = left_margin; pos < max; pos += 5)
    {
        if (((pos-left_margin) % 100) == 0)
        {
            QPolygon arrow(3);
            arrow.setPoint(0, pos, 2+2);
            arrow.setPoint(1, pos+3, 9+2);
            arrow.setPoint(2, pos-3, 9+2);
            painter->drawPolygon(arrow);

            Time time_here = Time::scale(model->min_time(), model->max_time(),
                                         double(pos-left_margin)/pixel_lenth);

            QString lti = time_here.toString();

            QFontMetrics fm(painter->font());
            int width = fm.width(lti);

            int x = pos-width/2;

            // Shifts the first and last time markers to be
            // in printable area (if printing).
            if (printer_flag) {
                if (pos == left_margin) x = pos-3;
                if (pos+5 >= max)       x = max-width;
            }

            painter->drawText( x, timeline_text_top, width, 100,
                               Qt::AlignTop | Qt::AlignHCenter, lti);
        }
        else if (((pos-left_margin) % 10) == 0)
        {
            painter->drawLine(pos, 3+2, pos, 6+2);
        }
    }

    painter->restore();
}

#undef NP

bool Trace_geometry::clickable_component(const QPoint& point, int & component) const
{
    typedef QPair<QRect, int> pt;
    foreach(const pt& p, clickable_components)
    {
        if (p.first.contains(point))
        {
            component = p.second;
            return true;
        }
    }

    return false;
}

State_model* Trace_geometry::clickable_state(const QPoint& point) const
{
    typedef QPair<QRect, boost::shared_ptr<State_model> > pt;
    foreach(const pt& p, states)
    {
        if (p.first.contains(point))
            return p.second.get();
    }
    return 0;
}

int Trace_geometry::componentAtPosition(const QPoint& point)
{
    typedef QPair<QRect, int> pt;
    foreach(const pt& p, lifeline_rects)
    {
        if (p.first.contains(point))
            return p.second;
    }

    return -1;
}

int Trace_geometry::componentLabelAtPos(const QPoint& point)
{
    typedef QPair<QRect, int> pt;
    foreach(const pt& p, componentlabel_rects)
    {
        if (p.first.contains(point))
            return p.second;
    }

    return -1;
}

}
