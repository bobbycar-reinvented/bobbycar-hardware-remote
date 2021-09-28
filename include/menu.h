#include <Arduino.h>

#define MENU_MAIN 0U
#define MENU_SELECT_BOBBYCAR 1U
#define MENU_CONNECTED_TO_BOBBYCAR 2U
#define MENU_CALIBRASTE_ANALOG_STICKS 3U

void scanForDevices();
void showBobbycarList(uint index);
int countBobbycarsFound();
void connectToBobbycar(uint index);
void resetScrollDisplay();
void showConnected(uint index);
String buildEmptyMessage();
void calibrationScreen();

class Menu
{
public:
    uint getMenu()
    {
        return menu_index;
    }

    void switchMenu(uint new_index)
    {
        menu_index = new_index;
        resetScrollDisplay();
        update_menu();
    }

    void confirm()
    {
        switch (menu_index)
        {
        case MENU_MAIN:
            scanForDevices();
            break;

        case MENU_SELECT_BOBBYCAR:
            Serial.printf("%s => %s\n", bobbycar_names[MENU_SELECT_BOBBYCAR_INDEX].c_str(), bobbycar_uuids[MENU_SELECT_BOBBYCAR_INDEX].toString().c_str());
            connectToBobbycar(MENU_SELECT_BOBBYCAR_INDEX);
            break;

        case MENU_CALIBRASTE_ANALOG_STICKS:
            calibrationScreen();
            break;
        }
    }

    void back()
    {
        switch (menu_index)
        {
        case MENU_MAIN:
            break;

        case MENU_SELECT_BOBBYCAR:
            switchMenu(MENU_MAIN);
            break;

        case MENU_CONNECTED_TO_BOBBYCAR:
            pLivestatsCharacteristic->unsubscribe();
            std::string emptyMessage = buildEmptyMessage().c_str();
            pRemotecontrolCharacteristic->writeValue(emptyMessage);
            delay(10);
            int returnCode = pClient->disconnect();
            if (returnCode == 0)
            {
                Serial.println("Disconnected successfully");
                switchMenu(MENU_MAIN);
            }
            else
            {
                Serial.printf("Disconnect failed. (%i)\n", returnCode);
            }
            break;
        }
    }

    void up()
    {
        switch (menu_index)
        {
        case MENU_MAIN:
            break;

        case MENU_SELECT_BOBBYCAR:
            if (MENU_SELECT_BOBBYCAR_INDEX >= 1 && MENU_SELECT_BOBBYCAR_SCROLL)
            {
                MENU_SELECT_BOBBYCAR_INDEX--;
                update_menu(false);
            }
            break;

        case MENU_CALIBRASTE_ANALOG_STICKS:
            switchMenu(MENU_MAIN);
            break;
        }
    }

    void down()
    {
        switch (menu_index)
        {
        case MENU_MAIN:
            switchMenu(MENU_CALIBRASTE_ANALOG_STICKS);
            break;

        case MENU_SELECT_BOBBYCAR:
            if (MENU_SELECT_BOBBYCAR_INDEX < (countBobbycarsFound() - 1) && MENU_SELECT_BOBBYCAR_SCROLL)
            {
                MENU_SELECT_BOBBYCAR_INDEX++;
                update_menu(false);
            }
            break;
        }
    }

    void set_bobby_sel_scroll(bool scroll)
    {
        MENU_SELECT_BOBBYCAR_SCROLL = scroll;
    }

    bool updateSticks()
    {
        return (menu_index == MENU_CONNECTED_TO_BOBBYCAR);
    }

private:
    void update_menu(bool reset = true)
    {
        switch (menu_index)
        {
        case MENU_MAIN:
            display.clear();
            display.setCursor(0, 0);
            display.print("  Press confirm ");
            display.setCursor(0, 1);
            display.print("     to scan    ");
            break;

        case MENU_SELECT_BOBBYCAR:
            display.clear();
            MENU_SELECT_BOBBYCAR_SCROLL = false;
            firstLineIndex = 0;
            secondLineIndex = 0;
            scrollTimer = 0;
            scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;
            if (reset)
                MENU_SELECT_BOBBYCAR_INDEX = 0;
            showBobbycarList(MENU_SELECT_BOBBYCAR_INDEX);
            break;

        case MENU_CONNECTED_TO_BOBBYCAR:
            display.clear();
            showConnected(MENU_SELECT_BOBBYCAR_INDEX);
            break;

        case MENU_CALIBRASTE_ANALOG_STICKS:
            display.clear();
            display.setCursor(0, 0);
            display.print("  Press confirm ");
            display.setCursor(0, 1);
            display.print("  to calibrate. ");
            break;

        default:
            display.clear();
            break;
        }
    }
    uint menu_index = 0;

    // BOBBYCAR SELECT MENU
    uint MENU_SELECT_BOBBYCAR_INDEX = 0;
    bool MENU_SELECT_BOBBYCAR_SCROLL = false;
};