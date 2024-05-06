#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>

#include <Adafruit_SSD1306.h>
#include <splash.h>

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define LIQUIDLCD

#define OLED_RESET 4

#define TRIG A2 //초음파 송신
#define ECHO A3 //초음파 수신

#define btn_UP 2 // UP
#define btn_OK 6 // OK
#define btn_DOWN 3 // DOWN

enum MODE
{
    mode_Measure = 0,
    mode_Setting,
    mode_Menu
};

//Member
#ifdef LIQUIDLCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
//0,0        15,0
//0,1        15,1
#else
Adafruit_SSD1306 lcd(128, 64, &Wire, OLED_RESET);
#endif


bool getBtnPress = false;
int menuIndex = 0;
int dataDiameter = 0;
int dataOffset = 0;
MODE dispMode = mode_Measure;

void setup()
{
    Serial.setTimeout(100);
	Serial.begin(9600);

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
    lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    lcd.clearDisplay();
    #endif

    //SetSettingData(0,0);
    delay(1000);
    GetSettingData();
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
        dataDiameter = 0;
        dataOffset = 0;
        Serial.println("0,0");
    }
    else
    {
        int splitNum = savedData.indexOf(',');

        dataDiameter = savedData.substring(0,splitNum).toInt();
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

    delay(200);

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

        case mode_Setting :
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
                menuIndex++;
                if(menuIndex > 1) menuIndex = 1;
            }
            else if(getDown)
            {
                menuIndex--;
                if(menuIndex < 0) menuIndex = 0;
            }
            break;

        default: break;
        }
    }
}

void ShowDisplay()
{
    switch (dispMode)
    {
    case mode_Measure :
        UltrasonicRun();
        break;

    case mode_Setting :
        break;

    case mode_Menu :
        #ifdef LIQUIDLCD
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Measure Mode");
        lcd.setCursor(0,1);
        lcd.print("Setting Mode");
        lcd.blink();

        if(menuIndex == 0) lcd.setCursor(0,0);
        else lcd.setCursor(0,1);
        #else
        lcd.clearDisplay();
        lcd.setTextSize(1);
        lcd.setTextColor(WHITE);
        lcd.setCursor(0,0);
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
    #endif

    long duration, distance;

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    duration = pulseIn(ECHO, HIGH);//us
    Serial.print("Duration : ");
    Serial.println(duration);

    distance = (34 * duration) / 100 / 2;

    Serial.print("DIstance : ");
    Serial.println(distance);
    //초음파속도 340m/s
    //34000cm/s = 340000mm/s
    //1S = 1000ms = 1000000us
    //거리 = 340000 * duration / 1000000 / 2

    String showVlaue = String(distance).substring(0,13);
    #ifdef LIQUIDLCD
    lcd.setCursor(0,0);
    lcd.print(showVlaue);
    #else
    #endif

    delay(200);
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
