#include "decls.h"
unsigned long weatherConnectionTime = 0;          // Last time you connected to the server, in milliseconds
WiFiClient weatherClient; // wifi client object
#define FORECAST_IN_PSRAM
bool weatherExpanded = true;
OW_forecast *forecast;


//Weather API
const char* weatherHost = WEATHER_HOST;
String owmLocation = WEATHER_LOCATION;
String owmKey = WEATHER_OWMKEY;

const unsigned long  weatherPostingInterval = 30L*60L*1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, this sets it to every 30-mins or 48/day

uint16_t winddir_now;
float pressure_now;
float humidity_now;
time_t sunrise_now;
time_t sunset_now;

const char* shortDOW[] = {"???", "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const char* shortMonth[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

const char* longDOW[] = {"???", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* longMonth[] = {"January", "Febuary", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

const char* windDirName[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};


static lv_obj_t * weatherContainer;
#ifndef BIGWEATHER
static lv_obj_t * weatherBtn;
static lv_obj_t * weatherBtnLbl;
static void weatherBtnAction(lv_event_t * event);
#endif
static lv_obj_t * mainIcon;
static lv_obj_t * mainLabel;
static lv_obj_t * windImg;
static lv_obj_t * windLabel;
static lv_obj_t * tempLabel;
static lv_obj_t * todayLabel;
static lv_obj_t * sunLbl;
static lv_obj_t * moonLbl;
static lv_obj_t * moonImg;

#ifndef BIGWEATHER
static lv_obj_t * day0Lbl;
static lv_obj_t * day0Img;
static lv_obj_t * day0Detail;
#if (TFT_WIDTH == 480)
static lv_obj_t * day0Wind;
#endif
#endif
static lv_obj_t * day1Lbl;
static lv_obj_t * day1Img;
static lv_obj_t * day1Detail;
static lv_obj_t * day1Wind;
static lv_obj_t * day2Lbl;
static lv_obj_t * day2Img;
static lv_obj_t * day2Detail;
static lv_obj_t * day2Wind;
static lv_obj_t * day3Lbl;
static lv_obj_t * day3Img;
static lv_obj_t * day3Detail;
static lv_obj_t * day3Wind;
static lv_obj_t * day4Lbl;
static lv_obj_t * day4Img;
static lv_obj_t * day4Detail;
static lv_obj_t * day4Wind;

void weatherGrownAction(lv_anim_t * a);
void weatherShrunkAction(lv_anim_t * a);
bool weatherRequest();
uint8_t parseWeather(int c);
void drawWeather();
uint8_t weekday(time_t t);
const lv_img_dsc_t* getIconImage(char* name);
const lv_img_dsc_t* getSmallIconImage(char* name);
int getNextDayIndex(void);
void drawForecast(uint8_t dayIndex, lv_obj_t* dayLbl, lv_obj_t* dayImg, lv_obj_t* dayDetail, lv_obj_t* dayWind);
void parseWeatherTag(uint8_t level, char* tag, char* val);

//Messages from the weather client
QueueHandle_t weatherGetQueue = NULL;
enum : uint8_t { WTH_RESULT, WTH_TIMEOUT };
struct weatherMessage{
    uint8_t     cmd;
} weatherRxMessage;


//Called from setup
void setupWeather() {
  //Allocate weather data in PSRAM (or not)
#ifdef FORECAST_IN_PSRAM  
  forecast = (OW_forecast*)ps_malloc(MAX_3HRS * sizeof(OW_forecast));
#else
  forecast = (OW_forecast*)malloc(MAX_3HRS * sizeof(OW_forecast));
#endif
  if(!weatherGetQueue) weatherGetQueue = xQueueCreate(10, sizeof(struct weatherMessage));
}

//Construct the weather widget
lv_obj_t * createWeatherWidget(lv_obj_t * parent) {
  static lv_style_t style_yellow;
  lv_style_init(&style_yellow);
  lv_style_set_text_color(&style_yellow, lv_col(YELLOW));

  static lv_style_t style_green;
  lv_style_init(&style_green);
  lv_style_set_text_color(&style_green, lv_col(LTGREEN));

  static lv_style_t style_font;
  lv_style_init(&style_font);
#if (TFT_WIDTH == 480)
  lv_style_set_text_font(&style_font, &lv_font_montserrat_16);
#else
  lv_style_set_text_font(&style_font, &lv_font_montserrat_14);
#endif
  weatherContainer = lv_obj_create(parent);
  lv_obj_add_style(weatherContainer, &style_groupbox, LV_PART_MAIN);
  lv_obj_clear_flag(weatherContainer, LV_OBJ_FLAG_SCROLLABLE);
  int width = lv_obj_get_content_width(parent);
#ifndef BIGWEATHER
  lv_obj_add_event_cb(weatherContainer, weatherBtnAction, LV_EVENT_CLICKED, NULL);
#if (TFT_WIDTH == 480)
  if (weatherExpanded) lv_obj_set_size(weatherContainer, width, 150);
  else lv_obj_set_size(weatherContainer, 118, 150); 
#else
  if (weatherExpanded) lv_obj_set_size(weatherContainer, width, 108);
  else lv_obj_set_size(weatherContainer, 84, 108);
#endif
  day0Lbl = lv_label_create(weatherContainer);
  lv_obj_add_style(day0Lbl, &style_font, LV_PART_MAIN);
  lv_obj_add_style(day0Lbl, &style_yellow, LV_PART_MAIN);
  lv_obj_set_style_text_align(day0Lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(day0Lbl, "");
  day0Img = lv_img_create(weatherContainer);
  lv_obj_set_size(day0Img, 50, 50);
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(day0Img, 25, 25);
#else
  lv_obj_set_pos(day0Img, 8, 15);
#endif
  lv_obj_align_to(day0Lbl, day0Img, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  day0Detail = lv_label_create(weatherContainer);
  lv_obj_add_style(day0Detail, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day0Detail, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(day0Detail, day0Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_label_set_text(day0Detail, "");
#if (TFT_WIDTH == 480)  
  day0Wind = createWindSpd(weatherContainer, 50, 10);
  lv_obj_align_to(day0Wind, day0Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif
#else 
  lv_obj_set_size(weatherContainer, width, 322);
  todayLabel = lv_label_create(weatherContainer);
  lv_obj_add_style(todayLabel, &style_biggestfont, LV_PART_MAIN);
  lv_obj_add_style(todayLabel, &style_yellow, LV_PART_MAIN);
  lv_obj_set_size(todayLabel, 308, 20);
  lv_obj_set_pos(todayLabel, 0, 2);
  lv_obj_set_style_text_align(todayLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(todayLabel, "");

  mainIcon = lv_img_create(weatherContainer);
  lv_obj_set_size(mainIcon, 100, 100);
  lv_obj_align_to(mainIcon, todayLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);         //Align next to the slider
  tempLabel = lv_label_create(weatherContainer);
  lv_obj_add_style(tempLabel, &style_biggestfont, LV_PART_MAIN);
  lv_obj_set_style_text_align(tempLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_size(tempLabel, 120, 50);
  lv_obj_align_to(tempLabel, mainIcon, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
  lv_label_set_text(tempLabel, "");
  mainLabel = lv_label_create(weatherContainer);
  lv_obj_add_style(mainLabel, &style_biggerfont, LV_PART_MAIN);
  lv_obj_set_size(mainLabel, 120, 50);
  lv_obj_set_style_text_align(mainLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(mainLabel, tempLabel, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
  lv_label_set_text(mainLabel, "");
  
#ifdef THEME_HIVIS 
  windImg = createWindDir(weatherContainer, 50, 70, lv_color_black().full);
#else
  windImg = createWindDir(weatherContainer, 50, 70, lv_color_hex(0x202020).full);
#endif
  lv_obj_align_to(windImg, tempLabel, LV_ALIGN_OUT_RIGHT_TOP, 15, 0);         //Align next to the slider
  windLabel = lv_label_create(weatherContainer);
  lv_obj_add_style(windLabel, &style_biggerfont, LV_PART_MAIN);
  lv_obj_set_style_text_align(windLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(windLabel, windImg, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
  lv_label_set_text(windLabel, "");

#endif

  day1Lbl = lv_label_create(weatherContainer);
  lv_obj_add_style(day1Lbl, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day1Lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(day1Lbl, "");
  day1Img = lv_img_create(weatherContainer);
  lv_obj_set_size(day1Img, 50, 50);
#ifndef BIGWEATHER
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(day1Img, 128, 25);
#else
  lv_obj_set_pos(day1Img, 76, 15);
#endif
#else
  lv_obj_set_pos(day1Img, 15, 150);  
#endif
  lv_obj_align_to(day1Lbl, day1Img, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  day1Detail = lv_label_create(weatherContainer);
  lv_obj_add_style(day1Detail, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day1Detail, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(day1Detail, day1Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_label_set_text(day1Detail, "");
#if (TFT_WIDTH == 480 || defined(BIGWEATHER))  
#ifdef BIGWEATHER
  day1Wind = createWindSpd(weatherContainer, 50, 10, lv_color_black().full);
#else
  day1Wind = createWindSpd(weatherContainer, 50, 10, lv_color_hex(0x101010).full);
#endif
  lv_obj_align_to(day1Wind, day1Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif

  day2Lbl = lv_label_create(weatherContainer);
  lv_obj_add_style(day2Lbl, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day2Lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(day2Lbl, "");
  day2Img = lv_img_create(weatherContainer);
  lv_obj_set_size(day2Img, 50, 50);
#ifndef BIGWEATHER
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(day2Img, 206, 25);
#else
  lv_obj_set_pos(day2Img, 132, 15);
#endif
#else
  lv_obj_set_pos(day2Img, 90, 150);
#endif
  lv_obj_align_to(day2Lbl, day2Img, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  day2Detail = lv_label_create(weatherContainer);
  lv_obj_add_style(day2Detail, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day2Detail, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(day2Detail, day2Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_label_set_text(day2Detail, "");
#if (TFT_WIDTH == 480 || defined(BIGWEATHER))  
#ifdef BIGWEATHER
  day2Wind = createWindSpd(weatherContainer, 50, 10, lv_color_black().full);
#else
  day2Wind = createWindSpd(weatherContainer, 50, 10, lv_color_hex(0x101010).full);
#endif
  lv_obj_align_to(day2Wind, day2Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif

  day3Lbl = lv_label_create(weatherContainer);
  lv_obj_add_style(day3Lbl, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day3Lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(day3Lbl, "");
  day3Img = lv_img_create(weatherContainer);
  lv_obj_set_size(day3Img, 50, 50);
#ifndef BIGWEATHER
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(day3Img, 284, 25);
#else
  lv_obj_set_pos(day3Img, 188, 15);
#endif
#else
  lv_obj_set_pos(day3Img, 165, 150);
#endif
  lv_obj_align_to(day3Lbl, day3Img, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  day3Detail = lv_label_create(weatherContainer);
  lv_obj_add_style(day3Detail, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day3Detail, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(day3Detail, day3Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_label_set_text(day3Detail, "");
#if (TFT_WIDTH == 480 || defined(BIGWEATHER))
#ifdef BIGWEATHER  
  day3Wind = createWindSpd(weatherContainer, 50, 10, lv_color_black().full);
#else
  day3Wind = createWindSpd(weatherContainer, 50, 10, lv_color_hex(0x101010).full);
#endif
  lv_obj_align_to(day3Wind, day3Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif

  day4Lbl = lv_label_create(weatherContainer);
  lv_obj_add_style(day4Lbl, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day4Lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(day4Lbl, "");
  day4Img = lv_img_create(weatherContainer);
  lv_obj_set_size(day4Img, 50, 50);
#ifndef BIGWEATHER
#if (TFT_WIDTH == 480)
  lv_obj_set_pos(day4Img, 362, 25);
#else
  lv_obj_set_pos(day4Img, 244, 15);
#endif
#else
  lv_obj_set_pos(day4Img, 240, 150);
#endif
  lv_obj_align_to(day4Lbl, day4Img, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  day4Detail = lv_label_create(weatherContainer);
  lv_obj_add_style(day4Detail, &style_font, LV_PART_MAIN);
  lv_obj_set_style_text_align(day4Detail, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(day4Detail, day4Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_label_set_text(day4Detail, "");
#if (TFT_WIDTH == 480 || defined(BIGWEATHER))
#ifdef BIGWEATHER  
  day4Wind = createWindSpd(weatherContainer, 50, 10, lv_color_black().full);
#else
  day4Wind = createWindSpd(weatherContainer, 50, 10, lv_color_hex(0x101010).full);
#endif
  lv_obj_align_to(day4Wind, day4Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif

#ifdef BIGWEATHER
  sunLbl = lv_label_create(weatherContainer);
  lv_obj_add_style(sunLbl, &style_font, LV_PART_MAIN);
  //lv_obj_set_style_text_align(sunLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(sunLbl, 15, 260);
  lv_label_set_text(sunLbl, "");

#ifdef THEME_HIVIS  
  moonImg = createMoon(weatherContainer, 50, 50, lv_color_black().full);
#else
  moonImg = createMoon(weatherContainer, 50, 50, lv_color_hex(0x101010).full);
#endif
  lv_obj_set_pos(moonImg, 130, 255);

  moonLbl = lv_label_create(weatherContainer);
  lv_obj_add_style(moonLbl, &style_font, LV_PART_MAIN);
  //lv_obj_set_style_text_align(moonLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(moonLbl, 188, 260);
  lv_label_set_text(moonLbl, "");
#else
  //Expand button on top
  weatherBtn = lv_btn_create(weatherContainer);
  if (weatherExpanded) {
#if (TFT_WIDTH == 480)
    lv_obj_set_pos(weatherBtn, 431, 0);
    lv_obj_set_size(weatherBtn, 16, 135);
#else
    lv_obj_set_pos(weatherBtn, 286, 0);
    lv_obj_set_size(weatherBtn, 16, 93);
#endif  
  } else {
#if (TFT_WIDTH == 480)
    lv_obj_set_pos(weatherBtn, 89, 0);
    lv_obj_set_size(weatherBtn, 16, 135);
#else
    lv_obj_set_pos(weatherBtn, 58, 0);
    lv_obj_set_size(weatherBtn, 16, 93);
#endif  
  }
  lv_obj_set_style_bg_opa(weatherBtn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_style(weatherBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(weatherBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(weatherBtn, weatherBtnAction, LV_EVENT_CLICKED, NULL);
  weatherBtnLbl = lv_label_create(weatherBtn);
  lv_obj_align(weatherBtnLbl, LV_ALIGN_CENTER, 0, 0);
  if (weatherExpanded) lv_label_set_text(weatherBtnLbl, LV_SYMBOL_LEFT);
  else lv_label_set_text(weatherBtnLbl, LV_SYMBOL_RIGHT);
  lv_obj_move_background(weatherContainer);    
#endif
  return weatherContainer;
}

#ifndef BIGWEATHER
//Weather button clicked
static void weatherBtnAction(lv_event_t * event) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, weatherContainer);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_width);
#pragma GCC diagnostic pop
  lv_anim_set_time(&a, 500);
  if (!weatherExpanded) {
#if (TFT_WIDTH == 480)
    lv_anim_set_values(&a, 118, 460);
#else 
    lv_anim_set_values(&a, 84, 312);
#endif
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);
    lv_anim_set_var(&a, weatherBtn);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
#pragma GCC diagnostic pop
#if (TFT_WIDTH == 480)
    lv_anim_set_values(&a, 89, 431);
#else
    lv_anim_set_values(&a, 58, 286);
#endif
    lv_anim_set_ready_cb(&a, weatherGrownAction);
    lv_anim_start(&a);
    lv_obj_move_foreground(weatherContainer);    
    weatherExpanded = true;
  } else {
#if (TFT_WIDTH == 480)    
    lv_anim_set_values(&a, 460, 118);
#else
    lv_anim_set_values(&a, 312, 84);
#endif
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    lv_anim_set_var(&a, weatherBtn);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
#pragma GCC diagnostic pop
#if (TFT_WIDTH == 480)
    lv_anim_set_values(&a, 431, 89);
#else
    lv_anim_set_values(&a, 286, 58);
#endif
    lv_anim_set_ready_cb(&a, weatherShrunkAction);
    lv_anim_start(&a);
    weatherExpanded = false;
  }
}

//Animation callbacks
void weatherGrownAction(lv_anim_t * a) {
  lv_label_set_text(weatherBtnLbl, LV_SYMBOL_LEFT);
}

//Animation callbacks
void weatherShrunkAction(lv_anim_t * a) {
  lv_label_set_text(weatherBtnLbl, LV_SYMBOL_RIGHT);
  lv_obj_move_background(weatherContainer);    
}
#endif

//Called from wifiHandle on first connect
void weatherBegin() {
  //First fetch in 20 seconds from first connect
  if (!weatherConnectionTime) weatherConnectionTime = millis() + 20000;
}

//Called from loop
// Receive messages from weather client and dispatch actions from here
void weatherHandle() {
  if (weatherGetQueue && xQueueReceive(weatherGetQueue, &weatherRxMessage, 0) == pdPASS){
    if (weatherRxMessage.cmd == WTH_RESULT) {
      createWeather();  //if not already
      drawWeather();
    }
    //Don't worry about timeout, retry is automatic
  }
}

//Fill the weather widget with forecast data
void drawWeather() {
  //tmElements_t nowtime;
  //breakTime(rtc.getEpoch(), nowtime);
  char temp[256];
#ifdef BIGWEATHER
  //Day of week
  snprintf(temp, 255, "%s %d %s %d", longDOW[weekday(utf())], getDay(), longMonth[getMonth()], getYear()); 
  lv_label_set_text(todayLabel, temp);
#else
#if (TFT_WIDTH == 480)
  snprintf(temp, 255, "%s %d %s", shortDOW[weekday(utf())], getDay(), shortMonth[getMonth()]); 
#else
  snprintf(temp, 255, "%d %s", getDay(), shortMonth[getMonth()]); 
#endif
  lv_label_set_text(day0Lbl, temp);
#endif
  //Main weather condition icon
  if (utf() > sunrise_now && utf() < sunset_now) forecast[0].icon[2] = 'd';
  else forecast[0].icon[2] = 'n';
#ifdef BIGWEATHER
  lv_img_set_src(mainIcon, getIconImage(forecast[0].icon));
  lv_obj_align_to(mainIcon, todayLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);         //Align next to the slider
#else
  lv_img_set_src(day0Img, getSmallIconImage(forecast[0].icon));
  lv_obj_align_to(day0Lbl, day0Img, LV_ALIGN_OUT_TOP_MID, 0, 0); 
  snprintf(temp, 255, "%s\n%d" LV_SYMBOL_DEGREE, forecast[0].main, (int)(forecast[0].temp + 0.5));
  lv_label_set_text(day0Detail, temp);
#endif

#ifdef BIGWEATHER
  //Temperature and humidity label
  snprintf(temp, 255, "%d C\n%d%%", (int)(forecast[0].temp + 0.5), (int)humidity_now); 
  lv_label_set_text(tempLabel, temp);
  lv_obj_align_to(tempLabel, mainIcon, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

  //Main weather description and pressure
  snprintf(temp, 255, "%s\n%d hPa", forecast[0].main, (int)pressure_now); 
  lv_label_set_text(mainLabel, temp);
  lv_obj_align_to(mainLabel, tempLabel, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);

  //Wind rose
  int windAngle = (winddir_now + 22.5) / 45;
  if (windAngle > 7) windAngle = 0;
  drawWindDir(windImg, winddir_now, forecast[0].speed, forecast[0].gust);
  lv_obj_align_to(windImg, tempLabel, LV_ALIGN_OUT_RIGHT_TOP, 15, 0);         //Align next to the slider

  //Wind rose label
  snprintf(temp, 255, "%d m/s %s", (int)forecast[0].speed, windDirName[windAngle]); 
  lv_label_set_text(windLabel, temp);
  lv_obj_align_to(windLabel, windImg, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

#else
  lv_obj_align_to(day0Detail, day0Img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
#if (TFT_WIDTH == 480)  
  drawWindSpd(day0Wind, forecast[0].speed, forecast[0].gust);
  lv_obj_align_to(day0Wind, day0Detail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif
#endif
#ifdef BIGWEATHER
  //Sunrise and sunset
  snprintf(temp, 255, "Sunrise %d:%02d\nSunset %d:%02d", localtime(&sunrise_now)->tm_hour, localtime(&sunrise_now)->tm_min, localtime(&sunset_now)->tm_hour, localtime(&sunset_now)->tm_min); 
  lv_label_set_text(sunLbl, temp);

  //Moon
  uint8_t illum = 0;
  float moon = moonDay(0, &illum);    //(0) is now
  drawMoon(moonImg, 1 - moon);

  //Moon label
  float days = ((1 - moon) + 0.5) * 29.53;
  if (days >= 29.53) days -= 29.53;
  snprintf(temp, 255, "%d%% Luminous\n%d Days to full", illum, (int)days); 
  lv_label_set_text(moonLbl, temp);
#endif
  //Forecast
  int8_t dayIndex = getNextDayIndex();
  drawForecast(dayIndex, day1Lbl, day1Img, day1Detail, day1Wind);  
  dayIndex += 8;
  drawForecast(dayIndex, day2Lbl, day2Img, day2Detail, day2Wind);  
  dayIndex += 8;
  drawForecast(dayIndex, day3Lbl, day3Img, day3Detail, day3Wind);  
  dayIndex += 8;
  drawForecast(dayIndex, day4Lbl, day4Img, day4Detail, day4Wind);  
}

//Find the first forecast index after midnight
int getNextDayIndex(void) {
  int index = 0;
  uint8_t today;
  today = weekday(utf());
  for (index = 0; index < 8; index++) {
    if (today != weekday(forecast[index].dt)) break;   
  }
  return index;
}

//Draw a single forecast column
void drawForecast(uint8_t dayIndex, lv_obj_t* dayLbl, lv_obj_t* dayImg, lv_obj_t* dayDetail, lv_obj_t* dayWind) {
  if (dayIndex >= MAX_DAYS * 8) return;
  lv_label_set_text(dayLbl, shortDOW[weekday(forecast[dayIndex + 4].dt)]);
  lv_img_set_src(dayImg, getSmallIconImage(forecast[dayIndex + 4].icon));
  lv_obj_align_to(dayLbl, dayImg, LV_ALIGN_OUT_TOP_MID, 0, 0);         //Align next to the slider
  
  // Find the temperature min and max during the day
  float tmax = -9999;
  float tmin =  9999;
  for (int i = 0; i < 8; i++) if (forecast[dayIndex + i].tmax > tmax) tmax = forecast[dayIndex + i].tmax;
  for (int i = 0; i < 8; i++) if (forecast[dayIndex + i].tmin < tmin) tmin = forecast[dayIndex + i].tmin;
  char detail[128];
#if (TFT_WIDTH == 480)
  snprintf(detail, 127, "%s\n%d" LV_SYMBOL_DEGREE " - %d" LV_SYMBOL_DEGREE, forecast[dayIndex + 4].main, (int)(tmin + 0.5), (int)(tmax + 0.5));
#else
  snprintf(detail, 127, "%s\n%d" LV_SYMBOL_DEGREE "-%d" LV_SYMBOL_DEGREE, forecast[dayIndex + 4].main, (int)(tmin + 0.5), (int)(tmax + 0.5));
#endif
  lv_label_set_text(dayDetail, detail);
  lv_obj_align_to(dayDetail, dayImg, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
#if (TFT_WIDTH == 480 || defined(BIGWEATHER))  
  drawWindSpd(dayWind, forecast[dayIndex + 4].speed, forecast[dayIndex + 4].gust);
  lv_obj_align_to(dayWind, dayDetail, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
#endif
}

const lv_img_dsc_t* getIconImage(char* name) {
  if (strcmp(name, "01d") == 0) return &clear_day;
  else if (strcmp(name, "01n") == 0) return &clear_night;
  else if (strcmp(name, "02d") == 0 || strcmp(name, "03d") == 0) return &partly_cloudy_day;
  else if (strcmp(name, "02n") == 0 || strcmp(name, "03n") == 0) return &partly_cloudy_night;
  else if (strcmp(name, "04d") == 0 || strcmp(name, "04n") == 0) return &cloudy;
  else if (strcmp(name, "09d") == 0 || strcmp(name, "09n") == 0) return &lightRain;
  else if (strcmp(name, "10d") == 0 || strcmp(name, "10n") == 0) return &rain;
  else if (strcmp(name, "11d") == 0 || strcmp(name, "11n") == 0) return &thunderstorm;
  else if (strcmp(name, "13d") == 0 || strcmp(name, "13n") == 0) return &snow;
  else if (strcmp(name, "50d") == 0 || strcmp(name, "50n") == 0) return &fog;
  else return &unknown;
}

const lv_img_dsc_t* getSmallIconImage(char* name) {
  if (strcmp(name, "01d") == 0) return &sclearday;
  else if (strcmp(name, "01n") == 0) return &sclearnight;
  else if (strcmp(name, "02d") == 0 || strcmp(name, "03d") == 0) return &spartlycloudyday;
  else if (strcmp(name, "02n") == 0 || strcmp(name, "03n") == 0) return &spartlycloudynight;
  else if (strcmp(name, "04d") == 0 || strcmp(name, "04n") == 0) return &scloudy;
  else if (strcmp(name, "09d") == 0 || strcmp(name, "09n") == 0) return &slightRain;
  else if (strcmp(name, "10d") == 0 || strcmp(name, "10n") == 0) return &srain;
  else if (strcmp(name, "11d") == 0 || strcmp(name, "11n") == 0) return &sthunderstorm;
  else if (strcmp(name, "13d") == 0 || strcmp(name, "13n") == 0) return &ssnow;
  else if (strcmp(name, "50d") == 0 || strcmp(name, "50n") == 0) return &sfog;
  else return &sunknown;
}

//-----------------------------------------------------
//Open Weather Map client
// Threaded interface

//Called from Services thread
bool weatherClientHandle() {
  struct weatherMessage weatherTxTaskMessage;
  static unsigned long timeout = 0;
  static uint8_t level = 0;
  static bool readingWeather = false;
  //IF time to connect AND wifi is connected start a request
  if (WiFi.status() == WL_CONNECTED && weatherConnectionTime && millis() > weatherConnectionTime) {
    weatherConnectionTime = millis() + weatherPostingInterval;
    Serial.print("> Connecting to "+String(weatherHost)+" .."); // start a new connection
    readingWeather = weatherRequest();
    if (readingWeather) timeout = millis() + WEATHER_TIMEOUT;
    else Serial.println(" failed!");
  }
  //Client is active
  if (readingWeather) {
    //Timeout?
    if (millis() > timeout) {  
      Serial.println(" timeout!");
      readingWeather = false;
      weatherClient.stop();
      weatherTxTaskMessage.cmd = WTH_TIMEOUT;
      while (!xQueueSend(weatherGetQueue, &weatherTxTaskMessage, 0)) vTaskDelay(1);   
    }
    //Data available? 
    if (weatherClient.available()) {
      while(weatherClient.available())
        level = parseWeather(weatherClient.read());
    }
    //End of data?
    else if(!weatherClient.connected()) {
      readingWeather = false;
      if (level == 0) {
        Serial.println(" OK.");
        weatherTxTaskMessage.cmd = WTH_RESULT;
        while (!xQueueSend(weatherGetQueue, &weatherTxTaskMessage, 0)) vTaskDelay(1);   
      }
      else Serial.println(" truncated!");
    }
  }
  return readingWeather;
}

//Connect to server and send request
bool weatherRequest() {
  const int httpPort = 80;
  weatherClient.stop();  // Clear any current connections
  if (!weatherClient.connect(weatherHost, httpPort)) return false;
  //OpenWeatherMaps API
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + owmLocation + "&APPID=" + owmKey;  
  weatherClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
  "Host: " + weatherHost + "\r\n" +
  "Connection: close\r\n\r\n");  
  return true;
}

//Cheap-style JSON parser
uint8_t parseWeather(int c) {
  static uint8_t curlyLevel = 0;
  static bool inTag = false;
  static bool getTag = true;
  static bool inValue = false;
  static bool getValue = false;
  static char tag[128];
  static char val[256];
  static uint8_t tagIndex = 0;
  static uint16_t valIndex = 0;
  if (c == -1) return 1;    //Propagate errors
  //Serial.print((char)c);
  if (curlyLevel) {
    if (getTag) {
      if (c == '"') {
        inTag = !inTag;
        if (inTag) {
          tag[0] = 0;
          tagIndex = 0;
        } else {
          getTag = false;
          inValue = false;
          getValue = false;
          val[0] = 0;
          valIndex = 0;
        }
      }
      else if (inTag) {          
        if (tagIndex < 127) {
          tag[tagIndex++] = (char)c;
          tag[tagIndex] = 0;
        }
      }
    } else {
      if (!getValue) {
        if (c == ':') getValue = true;
      } else {
        if (c == '"') inValue = !inValue;
        else if (!inValue && (c == '{' || c == '[' || c == ',' || c == '}')) {
          parseWeatherTag(curlyLevel, tag, val);
          getValue = false;
          getTag = true;
        }
        else if (valIndex < 255) {
          val[valIndex++] = (char)c;
          val[valIndex] = 0;
        }
      }
    }
  }
  if (c == '{') curlyLevel++;
  else if (c == '}') curlyLevel--;
  return curlyLevel;  
}

//tag=value come in here
void parseWeatherTag(uint8_t level, char* tag, char* val) {
  static uint16_t listIndex = 0;
  //Look for interesting tags and store the values 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
  //All entries
  if (strcmp(tag, "list") == 0) listIndex = 0;
  else if (strcmp(tag, "dt") == 0) forecast[listIndex].dt = atol(val);
  else if (strcmp(tag, "temp") == 0) forecast[listIndex].temp = atof(val) - 273.15;
  else if (strcmp(tag, "temp_min") == 0) forecast[listIndex].tmin = atof(val) - 273.15;
  else if (strcmp(tag, "temp_max") == 0) forecast[listIndex].tmax = atof(val) - 273.15;
  else if (strcmp(tag, "main") == 0) { strncpy(forecast[listIndex].main, val, 31); forecast[listIndex].main[31] = '\0'; }
  else if (strcmp(tag, "description") == 0) { strncpy(forecast[listIndex].description, val, 63); forecast[listIndex].description[63] = '\0'; }
  else if (strcmp(tag, "icon") == 0) { strncpy(forecast[listIndex].icon, val, 15); forecast[listIndex].icon[15] = '\0'; }
  else if (strcmp(tag, "speed") == 0) forecast[listIndex].speed = atof(val);
  else if (strcmp(tag, "gust") == 0) forecast[listIndex].gust = atof(val);
  else if (strcmp(tag, "dt_txt") == 0) {
    //End of entry, next entry coming..
    listIndex++;
    Serial.print(".");
  }
  
  //Occur only once
  else if (strcmp(tag, "sunrise") == 0) sunrise_now = atoi(val);
  else if (strcmp(tag, "sunset") == 0) sunset_now = atoi(val);
#pragma GCC diagnostic pop

  //Only from first entry
  if (listIndex == 0) { 
    if (strcmp(tag, "deg") == 0) winddir_now = atoi(val);
    else if (strcmp(tag, "pressure") == 0) pressure_now = atof(val);
    else if (strcmp(tag, "humidity") == 0) humidity_now = atoi(val);
  }
}


//-------------------------------------------------------------------
//Time/date helpers

/*
uint32_t julianDay(time_t date) {
  if (date == 0) date = rtc.getEpoch();
  tmElements_t mtt;
  breakTime(date, mtt);
  long y = tmYearToCalendar(mtt.Year);
  long m = mtt.Month;
  if (m > 2) {
    m = m - 3;
  } else {
    m = m + 9;
    y--;
  }
  long c = y / 100L;          // Compute century
  y -= 100L * c;
  return ((uint32_t)mtt.Day + (c * 146097L) / 4 + (y * 1461L) / 4 + (m * 153L + 2) / 5 + 1721119L);
}

float normalize(float v) {
  v -= floor(v); 
  if (v < 0) v += 1;
  return v;
}

float moonDay(time_t date) {
  return normalize((julianDay(date) - 2451550.1) / 29.530588853);
}
*/

#define PI  3.1415926535897932384626433832795
#define RAD (PI/180.0)
#define SMALL_FLOAT (1e-12)

double Julian(int year, int month, double day)
{
  int a, b = 0, c, e;
  if (month < 3) {
    year--;
    month += 12;
  }
  if (year > 1582 || (year == 1582 && month > 10) ||
      (year == 1582 && month == 10 && day > 15)) {
    a = year / 100;
    b = 2 - a + a / 4;
  }
  c = 365.25 * year;
  e = 30.6001 * (month + 1);
  return b + c + e + day + 1720994.5;
}

double sun_position(double j)
{
  double n, x, e, l, dl, v;
  int i;

  n = 360 / 365.2422 * j;
  i = n / 360;
  n = n - i * 360.0;
  x = n - 3.762863;
  if (x < 0) x += 360;
  x *= RAD;
  e = x;
  do {
    dl = e - .016718 * sin(e) - x;
    e = e - dl / (1 - .016718 * cos(e));
  } while (fabs(dl) >= SMALL_FLOAT);
  v = 360 / PI * atan(1.01686011182 * tan(e / 2));
  l = v + 282.596403;
  i = l / 360;
  l = l - i * 360.0;
  return l;
}

double moon_position(double j, double ls)
{
  double ms, l, mm, n, ev, sms, ae, ec;
  int i;

  /* ls = sun_position(j) */
  ms = 0.985647332099 * j - 3.762863;
  if (ms < 0) ms += 360.0;
  l = 13.176396 * j + 64.975464;
  i = l / 360;
  l = l - i * 360.0;
  if (l < 0) l += 360.0;
  mm = l - 0.1114041 * j - 349.383063;
  i = mm / 360;
  mm -= i * 360.0;
  n = 151.950429 - 0.0529539 * j;
  i = n / 360;
  n -= i * 360.0;
  ev = 1.2739 * sin((2 * (l - ls) - mm) * RAD);
  sms = sin(ms * RAD);
  ae = 0.1858 * sms;
  mm += ev - ae - 0.37 * sms;
  ec = 6.2886 * sin(mm * RAD);
  l += ev + ec - ae + 0.214 * sin(2 * mm * RAD);
  l = 0.6583 * sin(2 * (l - ls) * RAD) + l;
  return l;
}

uint16_t moon_phase(int year, int month, int day, double hour, uint8_t* illum)
{
  double j = Julian(year, month, (double)day + hour / 24.0) - 2444238.5;
  double ls = sun_position(j);
  double lm = moon_position(j, ls);

  *illum = 100.0 * ((1.0 - cos((lm - ls) * RAD)) / 2) + 0.5; // percent illuminated

  double t = lm - ls;
  if (t < 0) t += 360;
  return t;
}

float moonDay(time_t date, uint8_t* illum) {
  if (date == 0) date = utf();
  tm *mtt;
  mtt = localtime(&date);
  return moon_phase(mtt->tm_year + 1900, mtt->tm_mon, mtt->tm_mday, mtt->tm_hour, illum) / 360.0;
}


//given unix time t, returns day of week Sun-Sat as an integer 1-7
uint8_t weekday(time_t t) {
    //return (((t / 86400) + 4) % 7) + 1;
  tm *mt = localtime(&t); 
  return mt->tm_wday + 1;
}

/*
void printTime(tmElements_t* tm) {
  Serial.print(tm->Hour);
  printDigits(tm->Minute);
  printDigits(tm->Second);
  Serial.print(" ");
  Serial.print(tm->Day);
  Serial.print("/");
  Serial.print(tm->Month);
  Serial.print("/");
  Serial.print(tmYearToCalendar(tm->Year)); 
}
void printDigits(int digits){
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
*/