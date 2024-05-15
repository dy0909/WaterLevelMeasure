// install Adafruit BusIO
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>


#include <Adafruit_SSD1306.h>
#include <splash.h>

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//#define LIQUIDLCD
#define VSERSION 0.04

#define OLED_RESET 4

#define TRIG A2 //초음파 송신
#define ECHO A3 //초음파 수신

#define btn_UP 5 // UP
#define btn_OK 7 // OK
#define btn_DOWN 3 // DOWN

#define PI 3.14159265359

enum MODE
{
    mode_Measure = 0,
    mode_Setting,
    mode_Setting_Calibration,
    mode_Setting_Calibration_Start,
    mode_Setting_Diameter,  //지름
    mode_Setting_Target,
    mode_Menu
};

struct SettingData
{
    float dataDiameter = 800;   //mm
    float tankHeight = 1500;    //mm
    int target = 70;           //L
};

//Member
#ifdef LIQUIDLCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
//0,0        15,0
//0,1        15,1
#else
Adafruit_SSD1306 lcd(128, 64, &Wire, OLED_RESET);
#endif

SettingData setData;
bool getBtnPress = false;
bool dispCleared = false;
int menuIndex = 0;
int dataOffset = 0;
MODE dispMode = mode_Menu;

void setup()
{
    Serial.setTimeout(100);
	Serial.begin(115200);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    pinMode(btn_UP, INPUT_PULLUP);
    pinMode(btn_OK, INPUT_PULLUP);
    pinMode(btn_DOWN, INPUT_PULLUP);

    #ifdef LIQUIDLCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    #else
    lcd.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    lcd.clearDisplay();
    #endif

    //SetSettingData(0,0);
    delay(1000);
    //GetSettingData();
    Serial.println("setup end");
}

void loop()
{
    GetBtn();

    ShowDisplay();

    //delay(50);
}

#pragma region EEPROM Data Method
void GetSettingData()
{
    String savedData = "";

    for(int i=0; i<20; i++)
    {
        //if(EEPROM.read(i) == NULL) break;

        savedData += String(EEPROM.read(i));
    }

    Serial.println(savedData);

    if(savedData == "" || savedData.indexOf(',') == -1)
    {
        setData.dataDiameter = 0;
        dataOffset = 0;
        Serial.println("0,0");
    }
    else
    {
        int splitNum = savedData.indexOf(',');

        setData.dataDiameter = savedData.substring(0,splitNum).toInt();
        dataOffset = savedData.substring(splitNum + 1).toInt();
    }
}

void SetSettingData(int dim, int ofs)
{
    Serial.println("Rom Clear Start");
    for(int i=0; i<EEPROM.length(); i++)
    {
        EEPROM.write(i,0);
    }
    Serial.println("Rom Clear End");
}
#pragma endregion EEPROM Data Method

void GetBtn()
{
    bool getUp = !digitalRead(btn_UP);
    bool getDown = !digitalRead(btn_DOWN);
    bool getOk = !digitalRead(btn_OK);
    getBtnPress = getUp || getDown || getOk;

    if(!getBtnPress) return;

    delay(100);

    getUp = !digitalRead(btn_UP);
    getDown = !digitalRead(btn_DOWN);
    getOk = !digitalRead(btn_OK);
    getBtnPress = getUp || getDown || getOk;


    if(getBtnPress)
    {
        switch (dispMode)
        {
        case mode_Measure :
            if(getOk)
            {
                dispMode = mode_Menu;
                menuIndex = 0;
            }
            break;

        // mode_Setting_Calibration,
        // mode_Setting_Diameter,  //지름
        // mode_Setting_Target,
        case mode_Setting :
            if(getOk)
            {
                if(menuIndex == 0)
                {
                    dispMode = mode_Setting_Calibration;
                    menuIndex = 0;
                }
                else if(menuIndex == 1) dispMode = mode_Setting_Diameter;
                else dispMode = mode_Setting_Target;
            }
            else if(getUp)
            {
                menuIndex--;
                if(menuIndex < 0) menuIndex = 0;
                
            }
            else if(getDown)
            {
                menuIndex++;
                if(menuIndex > 2) menuIndex = 2;
            }
            break;

        case mode_Setting_Calibration :
            if(getOk)
            {
                if(menuIndex == 0)
                    dispMode = mode_Setting_Calibration_Start;
                else
                    dispMode = mode_Setting;

                menuIndex = 0;
            }
            else if(getUp)
            {
                menuIndex--;
                if(menuIndex < 0) menuIndex = 0;
                
            }
            else if(getDown)
            {
                menuIndex++;
                if(menuIndex > 1) menuIndex = 1;
            }
            break;

        case mode_Setting_Diameter :
            break;

        case mode_Setting_Target :
            break;

        case mode_Menu :
            if(getOk)
            {
                if(menuIndex == 0) dispMode = mode_Measure;
                else dispMode = mode_Setting;
                menuIndex = 0;
            }
            else if(getUp)
            {
                menuIndex--;
                if(menuIndex < 0) menuIndex = 0;
                
            }
            else if(getDown)
            {
                menuIndex++;
                if(menuIndex > 1) menuIndex = 1;
            }
            break;

            Serial.print("Index ");
            Serial.println(menuIndex);
        default: break;
        }

        while(true)
      {
        getUp = !digitalRead(btn_UP);
        getDown = !digitalRead(btn_DOWN);
        getOk = !digitalRead(btn_OK);
        getBtnPress = getUp || getDown || getOk;

        dispCleared = false;

        if(!getBtnPress) break;

        delay(10);
      }
    }
}

void ShowDisplay()
{
    if(!dispCleared)
    {
        #ifdef LIQUIDLCD
        lcd.clear();
        dispCleared = true;
        #else
        lcd.clearDisplay();
        lcd.setTextSize(1);
        lcd.setTextColor(WHITE);
        //dispCleared = true;
        #endif
    }

    switch (dispMode)
    {
    case mode_Measure :
        UltrasonicRun();
        break;

    case mode_Setting :
        break;

    case mode_Menu :
        #ifdef LIQUIDLCD
        lcd.setCursor(0,0);
        lcd.print("Measure Mode");
        lcd.setCursor(0,1);
        lcd.print("Setting Mode");
        lcd.blink();

        if(menuIndex == 0) lcd.setCursor(0,0);
        else lcd.setCursor(0,1);

        Serial.print("Index ");
            Serial.println(menuIndex);
        #else
        lcd.setTextSize(2);
        lcd.setCursor((lcd.width() - (12 * 4)) / 2, 0);
        lcd.println("MENU");
        lcd.drawLine(0, 16, lcd.width(), 16, WHITE);

        char *menuItems[] = {"MainScreen", "Setting"};

        for(int i=0; i<2; i++)
        {
            if(menuIndex == i)
            {
                lcd.fillRect(0, i * 22 + 20, lcd.width(), 16, WHITE);
                lcd.setTextColor(BLACK, WHITE);
            }
            else
            {
                lcd.setTextColor(WHITE, BLACK);
            }

            lcd.setCursor(0, i * 22 + 20);
            lcd.println(menuItems[i]);
        }

        lcd.display();
        #endif
        break;

    default: break;
    }
}

void UltrasonicRun()
{
    #ifdef LIQUIDLCD
    lcd.clear();
    lcd.setCursor(14,0);
    lcd.print("mm");
    #else
    lcd.setTextSize(1);
    #endif

    float duration = 0;
    float distance = 0;

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    duration = pulseIn(ECHO, HIGH);//us

    #ifdef LIQUIDLCD
    distance = (34 * duration) / 100 / 2;   //mm
    #else
    distance = duration * 17 / 1000;    //cm
    #endif

    // Serial.print("DIstance : ");
    // Serial.println(distance);

    //초음파속도 340m/s
    //34000cm/s = 340000mm/s
    //1S = 1000ms = 1000000us
    //거리 = 340000 * duration / 1000000 / 2

    float height = (setData.tankHeight / 10.0) - distance;
    float r = setData.dataDiameter / 20.0;
    float volume = (PI * r * r * height) / 10000.0f;

    if(volume < 0) volume = 0;

    #ifdef LIQUIDLCD
    String showVlaue = String(distance).substring(0,13);
    lcd.setCursor(0,0);
    lcd.print(showVlaue);
    #else
    lcd.setTextSize(2);
    lcd.setCursor((lcd.width() - 6 * 8) / 2, 15);
    lcd.print(volume, 1);
    lcd.print(" L");

    // Draw progress bar
    int progressBarWidth = lcd.width() - 4;
    int progressBarHeight = 12;
    int progressBarX = 2;
    int progressBarY = (lcd.height() - progressBarHeight) / 2 + 15;

    lcd.drawRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, WHITE);
    int progress = map(volume, 0, setData.target, 0, progressBarWidth);

    if(progress > progressBarWidth)
    {
        progress = progressBarWidth;
    }

    lcd.fillRect(progressBarX, progressBarY, progress, progressBarHeight, WHITE);

    lcd.display();

    delay(100);
    #endif
}

#pragma region SerialEvent
void serialEvent()
{
    Serial.println("event enter");

    if(Serial.available())
    {
        Serial.println("event enter");

        String recvBuffer = Serial.readString();

        Serial.println(recvBuffer);

        // if(recvBuffer == "ON") runSensor = true;
        // else if(recvBuffer == "OFF") runSensor = false;
    }
}
#pragma endregion SerialEvent
