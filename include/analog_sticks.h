#include <Arduino.h>

#define ANALOG_LEFT_X 0U
#define ANALOG_LEFT_Y 1U
#define ANALOG_LEFT_BUTTON 2U

#define ANALOG_RIGHT_X 3U
#define ANALOG_RIGHT_Y 4U
#define ANALOG_RIGHT_BUTTON 5U

#define WHEEL_FRONT_LEFT 0U
#define WHEEL_FRONT_RIGHT 1U
#define WHEEL_BACK_LEFT 2U
#define WHEEL_BACK_RIGHT 3U

#define STICK_MODE_BOTH 0U
#define STICK_MODE_LEFT 1U
#define STICK_MODE_RIGHT 2U

class Inputs
{
public:
    void setPWMs(int front_steer_pwm, int back_steer_pwm, int front_drive_pwm, int back_drive_pwm)
    {
        _front_steer_pwm = front_steer_pwm;
        _back_steer_pwm = back_steer_pwm;
        _front_drive_pwm = front_drive_pwm;
        _back_drive_pwm = back_drive_pwm;
    }

    int getWheelValue(int wheel)
    {
        auto x = getX();
        auto y = getY();
        switch (wheel)
        {
        case WHEEL_FRONT_LEFT:
            return (x * _front_steer_pwm) + (y * _front_drive_pwm);
            break;

        case WHEEL_FRONT_RIGHT:
            return (-x * _front_steer_pwm) + (y * _front_drive_pwm);
            break;

        case WHEEL_BACK_LEFT:
            return (x * _back_steer_pwm) + (y * _back_drive_pwm);
            break;

        case WHEEL_BACK_RIGHT:
            return (-x * _back_steer_pwm) + (y * _back_drive_pwm);
            break;

        default:
            return 0;
            break;
        }
    }

    void init()
    {
        pinMode(LEFT_X_PIN, INPUT);
        pinMode(LEFT_Y_PIN, INPUT);
        pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);

        pinMode(RIGHT_X_PIN, INPUT);
        pinMode(RIGHT_Y_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
    }

    int getAxisValue(uint function)
    {
        switch (function)
        {
        case ANALOG_LEFT_X:
            return LEFT_X_VAL;
            break;

        case ANALOG_LEFT_Y:
            return LEFT_Y_VAL;
            break;

        case ANALOG_RIGHT_X:
            return RIGHT_X_VAL;
            break;

        case ANALOG_RIGHT_Y:
            return RIGHT_Y_VAL;
            break;
        default:
            return 0;
            break;
        }
    }

    bool getButtonValue(uint function)
    {
        switch (function)
        {
        case ANALOG_LEFT_BUTTON:
            return LEFT_BUTTON_VAL;
            break;
        case ANALOG_RIGHT_BUTTON:
            return RIGHT_BUTTON_VAL;
            break;
        default:
            return false;
            break;
        }
    }

    void setPin(uint function, uint pin)
    {
        switch (function)
        {
        case ANALOG_LEFT_X:
            LEFT_X_PIN = pin;
            break;
        case ANALOG_LEFT_Y:
            LEFT_Y_PIN = pin;
            break;
        case ANALOG_LEFT_BUTTON:
            LEFT_BUTTON_PIN = pin;
            break;

        case ANALOG_RIGHT_X:
            RIGHT_X_PIN = pin;
            break;
        case ANALOG_RIGHT_Y:
            RIGHT_Y_PIN = pin;
            break;
        case ANALOG_RIGHT_BUTTON:
            RIGHT_BUTTON_PIN = pin;
            break;

        default:
            assert(0);
        }
    }

    void update()
    {
        auto raw_lx = analogRead(LEFT_X_PIN);
        auto raw_ly = analogRead(LEFT_Y_PIN);
        auto raw_rx = analogRead(RIGHT_X_PIN);
        auto raw_ry = analogRead(RIGHT_Y_PIN);

        LEFT_X_VAL = mapAnalogStick(lXMiddle, lXStart, lXEnd, raw_lx);
        LEFT_Y_VAL = mapAnalogStick(lYMiddle, lYStart, lYEnd, raw_ly);
        RIGHT_X_VAL = mapAnalogStick(rXMiddle, rXStart, rXEnd, raw_rx);
        RIGHT_Y_VAL = mapAnalogStick(rYMiddle, rYStart, rYEnd, raw_ry);

        LEFT_BUTTON_VAL = !digitalRead(LEFT_BUTTON_PIN);
        RIGHT_BUTTON_VAL = !digitalRead(RIGHT_BUTTON_PIN);
    }

    void setCalibrationValues(int leftXMiddle, int leftXStart, int leftXEnd, int leftYMiddle, int leftYStart, int leftYEnd, int rightXMiddle, int rightXStart, int rightXEnd, int rightYMiddle, int rightYStart, int rightYEnd)
    {
        lXMiddle = leftXMiddle;
        lXStart = leftXStart;
        lXEnd = leftXEnd;

        lYMiddle = leftYMiddle;
        lYStart = leftYStart;
        lYEnd = leftYEnd;

        rXMiddle = rightXMiddle;
        rXStart = rightXStart;
        rXEnd = rightXEnd;

        rYMiddle = rightYMiddle;
        rYStart = rightYStart;
        rYEnd = rightYEnd;
    }

private:
    int lXMiddle = LEFT_ANALOG_X_MIDDLE;
    int lXStart = LEFT_ANALOG_X_START;
    int lXEnd = LEFT_ANALOG_X_END;

    int lYMiddle = LEFT_ANALOG_Y_MIDDLE;
    int lYStart = LEFT_ANALOG_Y_START;
    int lYEnd = LEFT_ANALOG_Y_END;

    int rXMiddle = RIGHT_ANALOG_X_MIDDLE;
    int rXStart = RIGHT_ANALOG_X_START;
    int rXEnd = RIGHT_ANALOG_X_END;

    int rYMiddle = RIGHT_ANALOG_Y_MIDDLE;
    int rYStart = RIGHT_ANALOG_Y_START;
    int rYEnd = RIGHT_ANALOG_Y_END;

    float getX()
    {
        switch (analog_stick_mode)
        {
        case STICK_MODE_BOTH:
            return (LEFT_X_VAL / 100.f);
            break;

        case STICK_MODE_RIGHT:
            return (RIGHT_X_VAL / 100.f);
            break;

        default: // STICK_MODE_LEFT
            return (LEFT_X_VAL / 100.f);
            break;
        }
    }

    float getY()
    {
        switch (analog_stick_mode)
        {
        case STICK_MODE_BOTH:
            return (RIGHT_Y_VAL / 100.f);
            break;

        case STICK_MODE_RIGHT:
            return (RIGHT_Y_VAL / 100.f);
            break;

        default: // STICK_MODE_LEFT
            return (LEFT_Y_VAL / 100.f);
            break;
        }
    }

    int analog_stick_mode = STICK_MODE_LEFT;

    // PWMs
    int _front_steer_pwm = 100;
    int _back_steer_pwm = 0;
    int _front_drive_pwm = 75;
    int _back_drive_pwm = 100;

    // Left Analog stick
    // X
    int LEFT_X_PIN;
    int LEFT_X_VAL;
    // Y
    int LEFT_Y_PIN;
    int LEFT_Y_VAL;
    // Button
    int LEFT_BUTTON_PIN;
    bool LEFT_BUTTON_VAL;

    // Right Analog stick
    // X
    int RIGHT_X_PIN;
    int RIGHT_X_VAL;
    // Y
    int RIGHT_Y_PIN;
    int RIGHT_Y_VAL;
    // Button
    int RIGHT_BUTTON_PIN;
    bool RIGHT_BUTTON_VAL;

    int mapAnalogStick(int middle, int start, int end, int raw)
    {
        if (abs(raw - middle) < DEADBAND)
        {
            return 0;
        }
        else if (raw < middle)
        {
            auto return_val = map(raw, start, middle - DEADBAND, -100, 0);
            if (return_val > 0)
                return 0;
            if (return_val < -100)
                return -100;
            return return_val;
        }
        else
        {
            auto return_val = map(raw, middle + DEADBAND, end, 0, 100);
            if (return_val < 0)
                return 0;
            if (return_val > 100)
                return 100;
            return return_val;
        }
    }
};