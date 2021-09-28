#include <Arduino.h>

#define BUTTONS_CONFIRM 0
#define BUTTONS_BACK 1
#define BUTTONS_UP 2
#define BUTTONS_DOWN 3

class Buttons
{
public:
    void setPin(int function, int pin)
    {
        switch (function)
        {
        case BUTTONS_CONFIRM:
            confirmPin = pin;
            break;
        case BUTTONS_BACK:
            backPin = pin;
            break;
        case BUTTONS_UP:
            upPin = pin;
            break;
        case BUTTONS_DOWN:
            downPin = pin;
            break;

        default:
            assert(0);
        }
    }

    void init()
    {
        pinMode(confirmPin, INPUT_PULLUP);
        pinMode(backPin, INPUT_PULLUP);
        pinMode(upPin, INPUT_PULLUP);
        pinMode(downPin, INPUT_PULLUP);
    }

    void handle()
    {
        auto btn_confirm_pressed = !digitalRead(confirmPin);
        auto btn_back_pressed = !digitalRead(backPin);
        auto btn_up_pressed = !digitalRead(upPin);
        auto btn_down_pressed = !digitalRead(downPin);

        if (btn_confirm_pressed && confirm_last != btn_confirm_pressed)
        {
            confirmHandler();
        }
        confirm_last = btn_confirm_pressed;
        if (btn_back_pressed && back_last != btn_back_pressed)
        {
            backHandler();
        }
        back_last = btn_back_pressed;
        if (btn_up_pressed && up_last != btn_up_pressed)
        {
            upHandler();
        }
        up_last = btn_up_pressed;
        if (btn_down_pressed && down_last != btn_down_pressed)
        {
            downHandler();
        }
        down_last = btn_down_pressed;
    }

    void setCallback(int function, void (*handler)(void))
    {
        switch (function)
        {
        case BUTTONS_CONFIRM:
            confirmHandler = handler;
            break;
        case BUTTONS_BACK:
            backHandler = handler;
            break;
        case BUTTONS_UP:
            upHandler = handler;
            break;
        case BUTTONS_DOWN:
            downHandler = handler;
            break;

        default:
            assert(0);
        }
    }

private:
    int confirmPin;
    int backPin;
    int upPin;
    int downPin;

    bool confirm_last = false;
    bool back_last = false;
    bool up_last = false;
    bool down_last = false;

    void (*confirmHandler)(void) = [] {};
    void (*backHandler)(void) = [] {};
    void (*upHandler)(void) = [] {};
    void (*downHandler)(void) = [] {};
};