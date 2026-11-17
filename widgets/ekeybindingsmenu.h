#ifndef EKEYBINDINGSMENU_H
#define EKEYBINDINGSMENU_H

#include "eframedwidget.h"
#include "ekeybindings.h"

class eFramedButton;
class eLabel;

// Options menu opened from the Escape menu: click a key button,
// then press the new key (Escape cancels, Backspace clears).
class eKeyBindingsMenu : public eFramedWidget {
public:
    using eFramedWidget::eFramedWidget;
    ~eKeyBindingsMenu();

    void initialize(const eAction& closeAct);
protected:
    bool keyPressEvent(const eKeyPressEvent& e) override;
private:
    void startCapture(const eKeyAction action, const bool alt);
    void stopCapture();
    void updateTexts();
    void changeSpeed(const int by);

    struct eRow {
        eKeyAction fAction;
        eFramedButton* fKey = nullptr;
        eFramedButton* fAlt = nullptr;
    };
    std::vector<eRow> mRows;
    eLabel* mSpeedLabel = nullptr;
    eLabel* mHint = nullptr;

    bool mCapturing = false;
    eKeyAction mCapAction = eKeyAction::scrollUp;
    bool mCapAlt = false;
};

#endif // EKEYBINDINGSMENU_H
