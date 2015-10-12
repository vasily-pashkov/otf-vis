#ifndef TRACE_PAINTER_HPP_VP_2006_10_08
#define TRACE_PAINTER_HPP_VP_2006_10_08

#include "time_vis3.h"

#include <QPainter>
#include <QMap>

#include <boost/shared_ptr.hpp>

namespace vis4 {

class Trace_model;
class Trace_geometry;
class State_model;

/** Trace painter.
    Class encapsulates all common methods for drawing on screen
    and printing on printer.

    For screen case class has some relations with class
    Contents_widget. All common variables now placing in
    Trace_geometry object.

    For printer case method drawTrace make it possible to
    print not only visible, but any number of pages.
*/
class Trace_painter {

public: /* types */

    enum StateEnum { Ready, Active, Background, Canceled };

public: /* methods */

    /** Constructor. */
    Trace_painter();
    ~Trace_painter();

    /** Set model. */
    void setModel(boost::shared_ptr<Trace_model> & model);

    /** Set paint device. */
    void setPaintDevice(QPaintDevice * paintDevice);

    /** Draw the trace on the given paint device. */
    void drawTrace(const common::Time & timePerPage, bool start_in_background);

    int state() { return state_; }
    void setState(StateEnum state) { state_ = state; }

    /** Draws text in a nice frame.
        Uses current pen and brush for the frame, and black for text.

       @param text          Text for draw.
       @param painter       Painter for drawing.
       @param x             Absciss of top left corner of the frame.
       @param y             Ordinate of vertical middle of the frame.
       @param width         Width of the frame. If text does not fit,
                            it's truncated. Value may be -1, then width
                            will be auto-computed.
       @param height        Height of the frame.
       @param text_start_x  If the value is not -1, text is drawing starting
                            on that x position

       This function is public because one used not only in class Trace_painter.
    */
    static QRect drawTextBox(const QString & text,
                      QPainter* painter,
                      int x, int y, int width, int height,
                      int text_start_x = -1);

    /** Draws a time line using given painter and coordinates.

       @param painter       Painter for drawing.
       @param x             Absciss of the top left corner of the time line.
       @param y             Ordinate of the top left corner of the time line.
    */
    void drawTimeline(QPainter * painter, int x, int y);

    /** Calculates x coordinate corresponding to given time on timeline. */
    int pixelPositionForTime(const common::Time& time) const;

    /** Calculates Time corresponding to given pixel coordinate. */
    common::Time timeForPixel(int pixel_x) const;

    std::auto_ptr<Trace_geometry> traceGeometry() const;

private: /* methods */

    /** @name Group of methods that really draw parts of trace diagram. */
    //@{
    void drawComponentsList(int from_component, int to_component, bool drawLabels);
    void drawEvents(int from_component, int to_component);
    void drawStates(int from_component, int to_component);
    void drawGroups(int from_component, int to_component);
    //@}

    /** Calculates the number of pages, that must be printed. */
    void splitToPages();

    /** Draw(or print) the page with given number.
        @param i Page number by horizontal.
        @param j Page number by vertical.
    */
    void drawPage(int i, int j);

    /** Draws an arrow from (x1, y1) to (x2, y2) on 'painter'.
       The primary issue is that often, there are several arrows
       with the same start position, and zero delta_y. If we draw
       them as straight lines, they will overlap, and look ugly.
       We can't avoid overlap in general, but this case is very
       comment and better be handled.

       The approach is to make arrows with zero delta y curved.
       When delta y is large enough (or, more specifically, the
       angle to horisontal is small enough, arrow will be drawn as
       straigh line.
       We compute the angle of straight line between start and end
       to horisontal, and the compute "delta angle" -- angle to that
       straight line that will have start and end segments of the curve.
       For angle of 90 (delta y is zero), delta angle is 30.
       For a "limit angle" of 10, delta angle is 0 (the line is straight)
       For intermediate angles, we interporal.
    */

    /** Draw a marker with a page number.
       @param painter   Painter for drawing.
       @param page      Page number.
       @param count     Count of the pages.
       @param x         Marker X coordinate.
       @param y         Marker Y coordinate.
       @param horiz     Marker orientation (true - horizontal, false - vertical).
    */
    QRect drawPageMarker(QPainter * painter,
                        int page, int count,
                        int x, int y, bool horiz);

    void draw_unified_arrow(int x1, int y1, int x2, int y2, QPainter * painter,
                            bool always_straight = false,
                            bool start_arrowhead = false);

public: /* members */

    /** @name These values used for text drawing. */
    //@{
    unsigned int text_elements_height;
    unsigned int text_height;
    unsigned int text_letter_width;
    //@}

    int right_margin, left_margin;  ///< Width of left and right margins at the current page.

    int pages_horizontally, pages_vertically; ///< Number of pages for printing (sets by splitToPages() function).

    std::vector<unsigned int> lifeline_position;
    unsigned int lifeline_stepping;

    /** The height of timeline's markers */
    static const int timeline_text_top = 13;

private: /* members */

    boost::shared_ptr<Trace_model> model;      ///< Model for drawing.
    QPainter * painter;                 ///< Active painter.
    Trace_geometry * tg;                ///< Trace geometry (coords of component labels,
                                        ///< lifelines, states, events etc.

    bool printer_flag;          ///< Indicates paint device is QPrinter or not.

    int left_margin1;           ///< Width of margin at first page.
    int left_margin2;           ///< Width of margin at other pages.

    common::Time timePerFirstPage;
    common::Time timePerFullPage;
    common::Time timePerPage;                       ///< Trace scalling.

    uint components_per_page;

    int width, height;                      ///< Full paper (or screen widget) size, including margins.
    uint y_unparented;                      ///< Top margin for the components (however a parent label is placed above)
    uint timeline_height;

    /* How much even line stands out on top of states.  */
    static const int event_line_extra_height = 5;

    /* The difference in vertical coordinate of
       event line end point and the baseline of the
       letter drawn above it.  */
    static const int event_line_and_letter_spacing = 2;

    StateEnum state_;

    QMap<int, QColor> componentLabelColors;

};

/** Class holds all methods and members to manipulate with
    layouts of trace parts. Members of this class sets by
    Trace_painter class. And it's used by Content_widget class
    for handling mouse events.
*/
class Trace_geometry {

public: /* methods */

    /** Returns the pointer to the number of clickable component
        if point is withing the clickable area, and null otherwise. */
    bool clickable_component(const QPoint& point, int & component) const;

    State_model* clickable_state(const QPoint& point) const;

    int componentAtPosition(const QPoint & point);

    /** Function identify component's label, which rect contain point.
     *
     * @param point Interesting position.
     * @return Index of component in model_->component_names array,
     *         -1 when actual component is root, and -2 when no actual
     *         components around.
     *
     * Function used by tooltips mechanism
     */
    int componentLabelAtPos(const QPoint& p);

public: /* members */

    QVector< QVector<bool> > eventsNear;

private: /* members */

    /// Vector, holding component label's coordinates
    /// (used by tooltips mechanism)
    QVector<QPair<QRect, int> > componentlabel_rects;
    QVector<QPair<QRect, int> > lifeline_rects;

    QVector<QPair<QRect, int> > clickable_components;
    // FIXME: it might be bad to store all states here,
    // and better solution would be to re-get them from trace
    // on click.
    QVector<QPair<QRect, boost::shared_ptr<State_model> > > states;

    friend class Trace_painter;
};

/* This declaration is needed for stupid gcc-4.1 with broken name resolving alghoritm... :(  */

}
unsigned int qHash(const std::pair< std::pair<int, int>, std::pair<int, int> >&
                   p);

#endif
