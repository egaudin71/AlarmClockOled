#include <U8g2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include "RTClib.h"

//---------------- RTC MODULE
DS3231 rtc;
DateTime date_now;
DateTime date_before;

//---------------- DALLAS SENSOR
// Data wire is plugged into port 2 on the Arduino
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
// Pass our oneWire reference to Dallas Temperature.

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//---------------- OLED SCREEN
//U8G2_SSD1306_128X32_NONAME_1_HW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);   // All Boards without Reset of the Display
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED
#define BUTTON_NEXT 3
#define BUTTON_PREV 4
#define HEIGHT     32
#define WIDTH      128
#define debounce  250

#define bitmap_width 8
#define bitmap_height 12

static unsigned char degre_celsius_bits[] = {
  0x00, 0x00, 0x0e, 0x0a, 0xee, 0x20, 0x20, 0x20, 0xe0, 0x00, 0x00, 0x00
};
static unsigned char thermo_bits[] = {
  0x18, 0x24, 0x34, 0x24, 0x34, 0x24, 0x34, 0x24, 0x3c, 0x7e, 0x7e, 0x3c
};


char unit_time[] = "23:59:59";
char unit_date[] = "23/08/2020";

byte nbsensors = 0; // nb de capteurs detectes
//DeviceAddress AddressSensors[1];
DeviceAddress AddressSensors[1];
byte index = 0;

//----------------------- graphique et page
volatile byte page = 0;

//================================================
void change_page()
//================================================
{
  //----------------------- bouton et rebond
  static unsigned int last_button_time = 0;

  unsigned int button_time = millis();
  if (button_time - last_button_time > debounce)
  {
    last_button_time = button_time;
    if (page <= 1) {
      page++;
    }
    else{
      page =0;
    }
  }
}


//================================================
void draw(float value, byte index = 0)
//================================================
{
  u8g2.setFont(u8g2_font_fub17_tn);
  u8g2.setFontPosTop();

  byte h_txt = (HEIGHT - u8g2.getAscent()) >> 1;//division par 2
  byte w_txt = (WIDTH - u8g2.getStrWidth(printfloat2char(value))) >> 1;//division par 2

  u8g2.drawStr( w_txt, h_txt, printfloat2char(value));

  u8g2.setFont(u8g2_font_6x12_tr);
  char txt[] = " 9";
  txt[1] = 48 + index;
  //char txt='9';
  //txt = 48 + 0;
  u8g2.drawStr(0, 0, txt);
  u8g2.drawXBM( 16, 0, bitmap_width, bitmap_height, thermo_bits);
  u8g2.drawStr(0, 50, unit_time);
  u8g2.drawStr(WIDTH - u8g2.getStrWidth(unit_date), 50, unit_date);
}

//================================================
void drawClock()
//================================================
{
  u8g2.setFont(u8g2_font_fub17_tn);
  u8g2.setFontPosTop();

  byte h1 = u8g2.getAscent();
  byte h_txt = (HEIGHT - h1) ;//>> 1;//division par 2
  byte w_txt = (WIDTH - u8g2.getStrWidth(unit_time)) >> 1;//division par 2

  //  u8g2.drawStr( w_txt, h_txt, unit_time);
  u8g2.drawStr( w_txt, 0, unit_time);

  u8g2.setFont(u8g2_font_6x12_tr);
  h1 = u8g2.getAscent();
  w_txt = (WIDTH - u8g2.getStrWidth(unit_date)) >> 1;//division par 2
  u8g2.drawStr( w_txt, (HEIGHT - h1 - 4), unit_date);
}


//================================================
char *printfloat2char( float value )
//================================================
{
  static char txt[] = "  0.9";// ou  "100.9" ou "-20.9";
  float value_l = abs(value);

  byte vv = (byte)(value_l); //sans l'arrondi
  if (value < 0)  {
    txt[0] = 45; // pour le signe moins
  }
  else {
    txt[0] = ( (vv >= 100) ? (vv / 100 + 48) : 32); // 32 pour le car espace ou les centaines
  }

  txt[1] = ( (vv >= 10) ? ((vv % 100) / 10 + 48) : 32); // les dizaines
  txt[2] = vv % 10 + 48; // les unites
  // le txt[3] est le point
  txt[4] = (byte)(10 * (value_l - 100 * (byte)(value_l / 100) - (byte)(value_l) % 100)) + 48; // la decimale pour les nb infeieurs a 100
  return txt;

}

//================================================
char *printint2char( int value )
//================================================
{
  static char txt[] = "999";// ou  "100.9" ou "-20.9";

  byte vv = (byte)(value); //sans l'arrondi
  txt[0] = ( (vv >= 100) ? (vv / 100 + 48) : 32); // 32 pour le car espace ou les centaines
  txt[1] = ( (vv >= 10) ? ((vv % 100) / 10 + 48) : 32); // les dizaines
  txt[2] = vv % 10 + 48; // les unites
  return txt;

}

//========================== clocktime
void clocktick( byte &jours, byte &heures, byte &minutes, byte &secondes)
//=======================================
{
  secondes += 1;
  if ( secondes > 59 )// sinon rien on sort
  {
    minutes += 1;
    secondes = 0;
    if ( minutes > 59 )
    {
      heures += 1;
      minutes = 0;
      if ( heures > 23 )
      {
        jours += 1;
        heures = 0;
      }
    }
  }

}

//================================================
void setup(void)
//================================================
{
  //----------------------------------------- interruption
  attachInterrupt(1, change_page, HIGH);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  Serial.begin(9600);



  //----------------------------------------- detection du nombre de capteurs
  sensors.begin();
  nbsensors = sensors.getDeviceCount();
  /*  for (byte i = 0; i < nbsensors; i++) {
      sensors.getAddress(AddressSensors[i], i);
    }
  */
  sensors.getAddress(AddressSensors[0], 0);

  //----------------------------------------- boutons du graphique
  u8g2.begin(/*Select=*/ U8X8_PIN_NONE, /*Right/Next=*/ BUTTON_NEXT, /*Left/Prev=*/ BUTTON_PREV, /*Up=*/ U8X8_PIN_NONE, /*Down=*/ U8X8_PIN_NONE, /*Home/Cancel=*/ U8X8_PIN_NONE);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);

  Wire.begin();
  rtc.begin();
  
    if (! rtc.isrunning()) {
      //Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(__DATE__, __TIME__));
    }
  
  delay (2000);


  date_before = rtc.now();
  //Serial.println("initialization done.");

}

/*
   Main function, get and show the temperature
*/
//================================================
void loop(void)
//================================================
{
  unsigned long currenttime = millis();
  float temp[1];


  //DateTime
  date_now = rtc.now(); //recuperation date du RTC

  sensors.requestTemperatures(); // Send the command to get temperatures
  //  for (byte i = 0; i < nbsensors; i++) {
  temp[0] = sensors.getTempC(AddressSensors[0]);
  //  }

  if ( date_now.unixtime() > date_before.unixtime()) { // toutes les 2 secondes
    sprintf(unit_time, "%02d:%02d:%02d", date_now.hour(), date_now.minute(), date_now.second());
    sprintf(unit_date, "%02d/%02d/%04d", date_now.day(), date_now.month(), date_now.year());

    date_before = date_now;

  }



  // picture loop
  u8g2.firstPage();
  do {
    if (page == 2) {

      u8g2.setPowerSave(1); //sleep mode
    }
    if (page == 1) {
      u8g2.setPowerSave(0); //sleep mode

      drawClock();
    }
    if (page == 0) {
      u8g2.setPowerSave(0); //sleep mode
      draw(temp[0], 1);
    }

  } while ( u8g2.nextPage() );


}
