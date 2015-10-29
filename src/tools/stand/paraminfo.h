#ifndef PARAMINFO_H
#define PARAMINFO_H
#include "grapher.h"
#include <QDialog>
class QToolButton;
class QLabel;
class QTreeWidget;

namespace vis3 {
namespace stand {

/** Class for representation information about parameters around the cursor. */
class ParamInfoPopup : public QFrame {

    Q_OBJECT

public: /*  methods */

    ParamInfoPopup(QWidget * parent = 0);

    /** Shows/hides an info window. */
    void show(const Stand_update_event_list & params);

    QSize sizeHint () const;

private slots:

    void showParam();
    void nextParam();
    void previousParam();
    void showList(const QString &);

private: /* members */

    Stand_update_event_list params;
    int currentParam;

private: /* widgets */

    QToolButton * btnPrevious;
    QLabel * lblCounter;
    QToolButton * btnNext;
    QToolButton * btnClose;

    QLabel * lblParamInfo;
    QLabel * lblShowList;

};

class ParamInfoWnd : public QDialog {

    Q_OBJECT

public: /*  methods */

    ParamInfoWnd(const Stand_update_event_list & p, QWidget * parent = 0);

    QSize sizeHint () const;

private: /* widgets */

    QTreeWidget * paramList;

};

}

}
#endif
