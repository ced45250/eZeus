#ifndef EEVENTBACKGROUND_H
#define EEVENTBACKGROUND_H

#include "ewidget.h"

class eEventBackground : public eWidget {
public:
    using eWidget::eWidget;
    ~eEventBackground();

    // Is a dialog open?
    static bool sAnyOpen() { return sOpenCount > 0; }

    void initialize(eWidget* const parent,
                    eWidget* const child,
                    const bool closable,
                    const eAction& closeFunc);
protected:
    void paintEvent(ePainter& p);
    bool keyPressEvent(const eKeyPressEvent& e);
    bool mousePressEvent(const eMouseEvent& e);
private:
    void close();

    static int sOpenCount;
    bool mCounted = false;
    bool mClosable = false;
    eAction mCloseFunc;
};

#endif // EEVENTBACKGROUND_H
