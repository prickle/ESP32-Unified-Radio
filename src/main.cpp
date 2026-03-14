#include "decls.h"

//=======================================================================================
//---------------------------------------------------
//Main window

const char * codecString[] = {"", "WAV", "MP3", "AAC", "M4A", "FLAC", "OGG", "OFLC", "OPUS"};

static lv_obj_t * mainWindow;

//Main info labels
lv_obj_t * progNameLbl = NULL;
lv_obj_t * progNowLbl;
lv_obj_t * progTimeBar;
lv_obj_t * progTextLbl;

lv_obj_t * infoContainer;
lv_obj_t * bufLvlMeter;
lv_meter_indicator_t * bufLvlIndicator;
lv_obj_t * bufStatLbl;
//static lv_obj_t * spectrumChart;
lv_obj_t * reloadBtn;
lv_obj_t * loadSpinner;
lv_obj_t * fmFreqLbl;
lv_obj_t * dab_img;
lv_obj_t * dab_img_bar;
lv_obj_t * artImg;
lv_obj_t * artImg_bar;
lv_obj_t * seekDownBtn;
lv_obj_t * seekUpBtn;
lv_obj_t * receiverWidget;
lv_obj_t * weatherWidget;

lv_obj_t * signalWindow;
lv_obj_t * signalMeter;
lv_meter_indicator_t * signalIndic;

lv_obj_t * testKnob;
lv_meter_indicator_t * knobIndic;

static void reload_action(lv_event_t * event);
static void albumArt_action(lv_event_t * event);
static void seekDownAction(lv_event_t * event);
static void seekUpAction(lv_event_t * event);

//Main (middle) info window
void createMainWindow(lv_obj_t * win) {    
  mainWindow = win;

  lv_obj_set_scrollbar_mode(mainWindow, LV_SCROLLBAR_MODE_OFF);
  //---- Info Container ----------------
  
  //The main info container window
  infoContainer = lv_obj_create(mainWindow);
  //lv_cont_set_fit(infoContainer, LV_FIT_NONE);              //Fit the size to the content
#if defined(VUMETER) || defined(FFTMETER)
#if (TFT_WIDTH == 480)
#ifdef FFTMETER
  lv_obj_set_size(infoContainer, 460, 110);
#else
  lv_obj_set_size(infoContainer, 460, 102);
#endif
  lv_obj_set_pos(infoContainer, 4, 4);
#else  
#ifdef FFTMETER
  lv_obj_set_size(infoContainer, 312, 110);
#else
  lv_obj_set_size(infoContainer, 312, 102);
#endif
  lv_obj_set_pos(infoContainer, 0, 0);
#endif
#else
#if (TFT_WIDTH == 480)
  lv_obj_set_size(infoContainer, 460, 86);
  lv_obj_set_pos(infoContainer, 4, 4);
#else  
  lv_obj_set_size(infoContainer, 312, 86);
  lv_obj_set_pos(infoContainer, 0, 0);
#endif
#endif
  lv_obj_add_style(infoContainer, &style_groupbox, LV_PART_MAIN);
  lv_obj_clear_flag(infoContainer, LV_OBJ_FLAG_SCROLLABLE);
  //lv_obj_set_click(infoContainer, false);            //Allow page scroll

  static lv_style_t style_bar;
  lv_style_init(&style_bar);
  lv_style_set_bg_color(&style_bar, lv_color_hex(0xFF6025));

  //Create a style for the buffer level meter
  static lv_style_t style_lmeter;
  lv_style_init(&style_lmeter);
  lv_style_set_bg_opa(&style_lmeter, LV_OPA_0);
  lv_style_set_border_opa(&style_lmeter, LV_OPA_0);

  //DAB+ Logo
  dab_img = lv_img_create(infoContainer);
  lv_obj_remove_style_all(dab_img);
  lv_obj_set_pos(dab_img, 4, 5);  
  lv_obj_set_size(dab_img, 80, 60);
  lv_img_set_src(dab_img, &dabLogo);
  //lv_obj_set_hidden(dab_img, true);

  //Slideshow progress bar
  dab_img_bar = lv_bar_create(infoContainer);
  lv_obj_set_size(dab_img_bar, 80, 4);
  lv_obj_align_to(dab_img_bar, dab_img, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  //lv_bar_set_anim_time(dab_img_bar, 500);
  lv_bar_set_value(dab_img_bar, 0, LV_ANIM_OFF);
  lv_obj_add_style(dab_img_bar, &style_bar, LV_PART_INDICATOR);
  lv_obj_set_hidden(dab_img_bar, true);

  //Create a buffer level line meter 
  bufLvlMeter = lv_meter_create(infoContainer);
  lv_meter_scale_t * bufScale = lv_meter_add_scale(bufLvlMeter);
#ifdef THEME_BLUE
  lv_meter_set_scale_ticks(bufLvlMeter, bufScale, 31, 2, 8, lv_color_hex(0x505050));
#else
  lv_meter_set_scale_ticks(bufLvlMeter, bufScale, 31, 2, 8, lv_color_hex(0x404040));
#endif
  lv_meter_set_scale_range(bufLvlMeter, bufScale, 0, 100, 240, 150);
#ifdef THEME_BLUE
  bufLvlIndicator = lv_meter_add_scale_lines(bufLvlMeter, bufScale, lv_color_hex(0x04386c), lv_color_hex(0x91bfed), false, 0);
#else
  bufLvlIndicator = lv_meter_add_scale_lines(bufLvlMeter, bufScale, lv_palette_main(LV_PALETTE_RED), lv_palette_lighten(LV_PALETTE_GREEN, 1), false, 0);
#endif
  lv_meter_set_indicator_start_value(bufLvlMeter, bufLvlIndicator, 0);
  lv_meter_set_indicator_end_value(bufLvlMeter, bufLvlIndicator, 0);                       //Set the current value
  lv_obj_add_style(bufLvlMeter, &style_lmeter, LV_PART_MAIN);
  lv_obj_add_style(bufLvlMeter, &style_lmeter, LV_PART_INDICATOR);
  lv_obj_set_pos(bufLvlMeter, 0, 0);                        //Set the x coordinate
  lv_obj_set_size(bufLvlMeter, 86, 86);
  lv_obj_set_hidden(bufLvlMeter, true);

  //Buffer status label
  bufStatLbl = lv_label_create(infoContainer); //First parameters (scr) is the parent
  lv_label_set_text(bufStatLbl, "\n\n");  //Set the text
  lv_obj_align_to(bufStatLbl, bufLvlMeter, LV_ALIGN_CENTER, 2, 0);  //Align below the label
  lv_obj_set_style_text_align(bufStatLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_hidden(bufStatLbl, true);

  //Info labels
  progNameLbl = lv_label_create(infoContainer); //First parameters (scr) is the parent
  lv_label_set_long_mode(progNameLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
#if (TFT_WIDTH == 480) 
  lv_obj_set_size(progNameLbl, 346, LV_SIZE_CONTENT);
#else
  lv_obj_set_size(progNameLbl, 204, LV_SIZE_CONTENT);
#endif
  lv_obj_add_style(progNameLbl, &style_biggestfont, LV_PART_MAIN);
  lv_obj_set_style_anim_speed(progNameLbl, 20, LV_PART_MAIN);
  lv_label_set_text(progNameLbl, "");  //Set the text
  lv_obj_align_to(progNameLbl, bufLvlMeter, LV_ALIGN_OUT_RIGHT_TOP, 10, 5);  //Align below the label

  //Reload button
  reloadBtn = lv_btn_create(infoContainer);
  lv_obj_set_size(reloadBtn, 40, 35);
  lv_obj_add_style(reloadBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(reloadBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(reloadBtn, &style_bigfont_orange, LV_PART_SELECTED);
#if (TFT_WIDTH == 480)
  lv_obj_align_to(reloadBtn, bufLvlMeter, LV_ALIGN_OUT_RIGHT_TOP, 316, 0);
#else
  lv_obj_align_to(reloadBtn, bufLvlMeter, LV_ALIGN_OUT_RIGHT_TOP, 176, 0);
#endif
  lv_obj_add_event_cb(reloadBtn, reload_action, LV_EVENT_CLICKED, NULL);
  lv_obj_t * label = lv_label_create(reloadBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_REFRESH);
  lv_obj_set_hidden(reloadBtn, true);

  //Loading spinner
  loadSpinner = lv_spinner_create(infoContainer, 1000, 60);
  lv_obj_set_size(loadSpinner, 35, 35);
#if (TFT_WIDTH == 480)
  lv_obj_align_to(loadSpinner, bufLvlMeter, LV_ALIGN_OUT_RIGHT_TOP, 321, -1);
#else
  lv_obj_align_to(loadSpinner, bufLvlMeter, LV_ALIGN_OUT_RIGHT_TOP, 181, -1);
#endif
  lv_obj_set_hidden(loadSpinner, true);

  progNowLbl = lv_label_create(infoContainer); //First parameters (scr) is the parent
#if (TFT_WIDTH == 480)
  lv_obj_set_size(progNowLbl, 346, LV_SIZE_CONTENT);
#else
  lv_obj_set_size(progNowLbl, 204, LV_SIZE_CONTENT);
#endif
  lv_obj_add_style(progNowLbl, &style_bigfont, LV_PART_MAIN);
  lv_label_set_long_mode(progNowLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_align_to(progNowLbl, progNameLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  //Align below the label
  lv_label_set_text(progNowLbl, "");  //Set the text
  
  progTimeBar = lv_bar_create(infoContainer);
#if (TFT_WIDTH == 480)
  lv_obj_set_size(progTimeBar, 346, 10);
#else
  lv_obj_set_size(progTimeBar, 204, 10);
#endif
  lv_obj_align_to(progTimeBar, progNowLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);  //Align below the label
  lv_bar_set_range(progTimeBar, 0, 100);
  lv_bar_set_value(progTimeBar, 0, LV_ANIM_OFF);
  lv_obj_set_hidden(progTimeBar, true);
  
  progTextLbl = lv_label_create(infoContainer); //First parameters (scr) is the parent
#if (TFT_WIDTH == 480)
  lv_obj_set_size(progTextLbl, 346, LV_SIZE_CONTENT);
#else
  lv_obj_set_size(progTextLbl, 204, LV_SIZE_CONTENT);
#endif
  lv_obj_add_style(progTextLbl, &style_bigfont, LV_PART_MAIN);
  lv_label_set_long_mode(progTextLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_align_to(progTextLbl, progNowLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  lv_label_set_text(progTextLbl, "");  //Set the text

#ifdef VUMETER
#if (TFT_WIDTH == 480)
  vuMeter = createVU(infoContainer, 442, 6, lv_color_hex(0x101010).full);
#else
  vuMeter = createVU(infoContainer, 312, 6, lv_color_hex(0x101010).full);
#endif
  lv_obj_set_pos(vuMeter, 4, 76);
#endif

#ifdef FFTMETER
#if (TFT_WIDTH == 480)
  fftMeter = createFFT(infoContainer, 434, 24, lv_color_hex(0x101010).full);
#else
  fftMeter = createFFT(infoContainer, 294, 24, lv_color_hex(0x101010).full);
#endif
  lv_obj_set_pos(fftMeter, 4, 72);
#endif


  artImg = lv_img_create(mainWindow);
  lv_obj_align_to(artImg, infoContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  lv_obj_set_style_text_color(artImg, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
  lv_obj_set_style_text_align(artImg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(artImg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(artImg, albumArt_action, LV_EVENT_CLICKED, NULL);

  artImg_bar = lv_bar_create(mainWindow);
  lv_obj_set_size(artImg_bar, 80, 10);
  lv_obj_align_to(artImg_bar, infoContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
  lv_bar_set_value(artImg_bar, 0, LV_ANIM_OFF);
  lv_obj_add_style(artImg_bar, &style_bar, LV_PART_INDICATOR);
  lv_obj_set_hidden(artImg_bar, true);

  fmFreqLbl = lv_label_create(infoContainer); //First parameters (scr) is the parent
#ifdef FMSEEK  
  lv_obj_set_pos(fmFreqLbl, 0, 8);  
#else
  lv_obj_set_pos(fmFreqLbl, 0, 22);  
#endif
  lv_obj_set_size(fmFreqLbl, 96, 32);
  lv_label_set_text(fmFreqLbl, "");  //Set the text
  lv_label_set_long_mode(fmFreqLbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(fmFreqLbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_add_style(fmFreqLbl, &style_digitalfont, LV_PART_MAIN);
  lv_obj_set_hidden(fmFreqLbl, true);

  //Create a seek button
  seekDownBtn = lv_btn_create(infoContainer);
  lv_obj_set_size(seekDownBtn, 40, 25);   //Set the size
  lv_obj_add_style(seekDownBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(seekDownBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(seekDownBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(seekDownBtn, fmFreqLbl, LV_ALIGN_OUT_BOTTOM_LEFT, 3, 6);  //Align below the label
  lv_obj_add_event_cb(seekDownBtn, seekDownAction, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(seekDownBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_PREV);
  lv_obj_set_hidden(seekDownBtn, true);

  //Create another seek button
  seekUpBtn = lv_btn_create(infoContainer);
  lv_obj_set_size(seekUpBtn, 40, 25);   //Set the size
  lv_obj_add_style(seekUpBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(seekUpBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(seekUpBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(seekUpBtn, seekDownBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);         //Align next to the slider
  lv_obj_add_event_cb(seekUpBtn, seekUpAction, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(seekUpBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_NEXT);
  lv_obj_set_hidden(seekUpBtn, true);

#ifdef SCANNER
  receiverWidget = createReceiverWidget(mainWindow);
#if (TFT_HEIGHT == 240)
  lv_obj_align_to(recieverWidget, infoContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);  
#else
  lv_obj_align_to(receiverWidget, infoContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);  
#endif
  lv_obj_set_hidden(receiverWidget, true);
#endif

  //Construct a moving-coil style meter
  signalWindow = lv_obj_create(infoContainer);
  lv_obj_set_style_bg_opa(signalWindow, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(signalWindow, LV_OPA_TRANSP, LV_PART_MAIN);  
  //lv_obj_set_style_border_color(signalWindow, lv_palette_main(LV_PALETTE_BLUE_GREY), LV_PART_MAIN);
  lv_obj_set_size(signalWindow, 90, 40);
  lv_obj_align(signalWindow, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_clear_flag(signalWindow, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_update_layout(signalWindow);
  float sigW = lv_obj_get_content_width(signalWindow) / 10;
  label = lv_label_create(signalWindow);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, sigW);
  lv_label_set_text(label, "Signal");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN);

  signalMeter = lv_meter_create(signalWindow);
  lv_obj_set_style_bg_opa(signalMeter, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(signalMeter, LV_OPA_TRANSP, LV_PART_MAIN);  
  lv_obj_set_pos(signalMeter, -sigW * 3, -4);
  lv_obj_set_size(signalMeter, sigW * 16 , sigW * 16);

  // Add a scale first
  lv_meter_scale_t * sigScale = lv_meter_add_scale(signalMeter);
  lv_meter_set_scale_ticks(signalMeter, sigScale, 21, 2, 5, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(signalMeter, sigScale, 4, 3, 8, lv_color_white(), 100);
  lv_meter_set_scale_range(signalMeter, sigScale, 0, 100, 90, -135);
  
  lv_meter_indicator_t * indic;
  // Make the tick lines red at the end of the scale
  indic = lv_meter_add_scale_lines(signalMeter, sigScale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(signalMeter, indic, 80);
  lv_meter_set_indicator_end_value(signalMeter, indic, 100);

  // Add a needle line indicator
  signalIndic = lv_meter_add_needle_line(signalMeter, sigScale, 3, lv_palette_main(LV_PALETTE_ORANGE), -5);
  lv_obj_set_hidden(signalWindow, true);

//--------------------------------------------
/*
  testKnob = lv_meter_create(mainWindow);
  lv_obj_set_style_bg_opa(testKnob, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(testKnob, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_border_opa(testKnob, LV_OPA_0, LV_PART_MAIN); 
  lv_obj_set_style_pad_all(testKnob, 0, LV_PART_MAIN); 
  //lv_obj_set_style_border_color(testKnob, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);  
  lv_obj_set_pos(testKnob, 50, 350);
  lv_obj_set_size(testKnob, 50 , 50);

  // Add a scale first
  lv_meter_scale_t * knobScale = lv_meter_add_scale(testKnob);
  lv_meter_set_scale_ticks(testKnob, knobScale, 0, 2, 5, lv_palette_main(LV_PALETTE_GREY));
  //lv_meter_set_scale_major_ticks(testKnob, knobScale, 4, 3, 8, lv_color_white(), 100);
  lv_meter_set_scale_range(testKnob, knobScale, -20, 120, 360, 90);
  
//  lv_meter_indicator_t * indic;
  // Make the tick lines red at the end of the scale
//  indic = lv_meter_add_scale_lines(signalMeter, sigScale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
//  lv_meter_set_indicator_start_value(signalMeter, indic, 80);
//  lv_meter_set_indicator_end_value(signalMeter, indic, 100);


  // Add a blue arc to the start
  lv_meter_indicator_t * knobArc = lv_meter_add_arc(testKnob, knobScale, 3, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
  lv_meter_set_indicator_start_value(testKnob, knobArc, 0);
  lv_meter_set_indicator_end_value(testKnob, knobArc, 100);

  lv_meter_indicator_t * knobCirc = lv_meter_add_arc(testKnob, knobScale, 1, lv_palette_main(LV_PALETTE_BLUE), -7);
  lv_meter_set_indicator_start_value(testKnob, knobCirc, -20);
  lv_meter_set_indicator_end_value(testKnob, knobCirc, 120);

  // Add a needle line indicator
  knobIndic = lv_meter_add_arc(testKnob, knobScale, 15, lv_palette_main(LV_PALETTE_BLUE), -7);
  lv_meter_set_indicator_start_value(testKnob, knobIndic, 0);
  lv_meter_set_indicator_end_value(testKnob, knobIndic, 5);
*/
}

void realignMainWindow() {
  lv_obj_t* above = infoContainer;
#ifdef SCANNER
  bool receiverShowing = (settings->mode == MODE_NFM || settings->mode == MODE_NLW || settings->mode == MODE_NMW || settings->mode == MODE_NSW);
  lv_obj_set_hidden(receiverWidget, !receiverShowing);
  if (receiverShowing) above = receiverWidget;
#endif
  if (weatherWidget) {
#if (TFT_HEIGHT == 240)
    lv_obj_align_to(weatherWidget, above, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
#else
    lv_obj_align_to(weatherWidget, above, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
  }
#endif
}

void createWeather() {
  if (!weatherWidget) {
    weatherWidget = createWeatherWidget(mainWindow);
    realignMainWindow();
  }
}

void setStaticLabels(bool isStatic) {
  if (isStatic) {
    lv_label_set_long_mode(progNameLbl, LV_LABEL_LONG_CLIP);
    lv_label_set_long_mode(progNowLbl, LV_LABEL_LONG_CLIP);
    lv_label_set_long_mode(progTextLbl, LV_LABEL_LONG_DOT);
  }
  else {
    lv_label_set_long_mode(progNameLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(progNowLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(progTextLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  }
}

void setSignalMeterHidden(bool hide) {
  if (hide) {
#if (TFT_WIDTH == 480)
    lv_obj_set_size(progTextLbl, 346, LV_SIZE_CONTENT);
    lv_obj_set_size(progNowLbl, 346, LV_SIZE_CONTENT);
#else
    lv_obj_set_size(progTextLbl, 204, LV_SIZE_CONTENT);
    lv_obj_set_size(progNowLbl, 204, LV_SIZE_CONTENT);
#endif
  } else {
#if (TFT_WIDTH == 480)
    lv_obj_set_size(progTextLbl, 256, LV_SIZE_CONTENT);
    lv_obj_set_size(progNowLbl, 256, LV_SIZE_CONTENT);
#else
    lv_obj_set_size(progTextLbl, 114, LV_SIZE_CONTENT);
    lv_obj_set_size(progNowLbl, 114, LV_SIZE_CONTENT);
#endif
  }
  lv_obj_set_hidden(signalWindow, hide);
}

void setMainVisibility() {
#ifdef SDPLAYER
  lv_obj_set_hidden(bufLvlMeter, settings->mode != MODE_WEB && settings->mode != MODE_SD && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);
  lv_obj_set_hidden(bufStatLbl, settings->mode != MODE_WEB && settings->mode != MODE_SD && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);
  lv_obj_set_hidden(progTimeBar, settings->mode != MODE_SD  && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);  
  lv_obj_set_hidden(progTextLbl, settings->mode == MODE_SD || settings->mode == MODE_FTP || settings->mode == MODE_POD || settings->mode == MODE_DLNA);  
#else
  lv_obj_set_hidden(bufLvlMeter, settings->mode != MODE_WEB && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);
  lv_obj_set_hidden(bufStatLbl, settings->mode != MODE_WEB && settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);
  lv_obj_set_hidden(progTimeBar, settings->mode != MODE_FTP && settings->mode != MODE_POD && settings->mode != MODE_DLNA);  
  lv_obj_set_hidden(progTextLbl, settings->mode == MODE_FTP || settings->mode == MODE_POD || settings->mode == MODE_DLNA);  
#endif
  lv_obj_set_hidden(dab_img, settings->mode != MODE_DAB);
  lv_obj_set_hidden(dab_img_bar, settings->mode != MODE_DAB);
  lv_obj_set_hidden(fmFreqLbl, settings->mode != MODE_FM && settings->mode != MODE_NFM && settings->mode != MODE_NMW && settings->mode != MODE_NLW && settings->mode != MODE_NSW);
#ifdef FMSEEK  
  lv_obj_set_hidden(seekDownBtn, settings->mode != MODE_FM && settings->mode != MODE_NFM && settings->mode != MODE_NMW && settings->mode != MODE_NLW && settings->mode != MODE_NSW);
  lv_obj_set_hidden(seekUpBtn, settings->mode != MODE_FM && settings->mode != MODE_NFM && settings->mode != MODE_NMW && settings->mode != MODE_NLW && settings->mode != MODE_NSW);
#endif
#ifdef MONKEYBOARD
  lv_obj_set_hidden(vuMeter, settings->mode == MODE_DAB || settings->mode == MODE_FM);
#endif  
  setSignalMeterHidden(settings->mode != MODE_NFM && settings->mode != MODE_NMW && settings->mode != MODE_NLW && settings->mode != MODE_NSW);
  realignMainWindow();
}

//Reload button clicked
static void reload_action(lv_event_t * event) {
  //?reload action
#ifdef SDPLAYER
  if (settings->mode == MODE_SD) sdReinit();
#endif
}

static void albumArt_action(lv_event_t * event) {
  artAction();
}

void updateArtImg(lv_img_dsc_t * imgDsc, bool fullscreen) {
  lv_img_set_src(artImg, imgDsc);
  if (fullscreen) lv_obj_align_to(artImg, mainWindow, LV_ALIGN_CENTER, 0, 0);
  else lv_obj_align_to(artImg, infoContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);      
  //Ugly hack, keep things responsive
  //TODO: write my own drawJpg
  lv_task_handler();
  webradioHandle();
#ifdef VUMETER    
  VUHandle();
#endif  
}

void updateArtImg(const char * msg) {
  lv_img_set_src(artImg, msg);
  lv_obj_align_to(artImg, infoContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);      
}

void setArtBar(int pct) {
  if (artImg_bar) {
    if (pct == 0) {
      lv_bar_set_value(artImg_bar, 0, LV_ANIM_OFF);
      lv_obj_set_hidden(artImg_bar, true);
    } else {
      lv_obj_set_hidden(artImg_bar, false);
      lv_bar_set_value(artImg_bar, pct, LV_ANIM_ON);
    }
  }
}

//Put up an animated spinner while waiting
void showLoadSpinner() {
#if (TFT_WIDTH == 480)
  if (progNameLbl) lv_obj_set_size(progNameLbl, 304, 25);
#else
  if (progNameLbl) lv_obj_set_size(progNameLbl, 162, 25);
#endif
  lv_obj_set_hidden(reloadBtn, true);
  lv_obj_set_hidden(loadSpinner, false);
}

// Show the reload button if connection issues
void showReloadBtn() {
#if (TFT_WIDTH == 480)  
  if (progNameLbl) lv_obj_set_size(progNameLbl, 299, 25);
#else
  if (progNameLbl) lv_obj_set_size(progNameLbl, 157, 25);
#endif
  lv_obj_set_hidden(loadSpinner, true);
  lv_obj_set_hidden(reloadBtn, false);
}
    
//Hide the spinner and reload buttons
void hideWebControls() {
#if (TFT_WIDTH == 480)  
  if (progNameLbl) lv_obj_set_size(progNameLbl, 346, 25);
#else
  if (progNameLbl) lv_obj_set_size(progNameLbl, 204, 25);
#endif
  lv_obj_set_hidden(loadSpinner, true);
  lv_obj_set_hidden(reloadBtn, true);
}

void clearProgLbl() {
  info(NAME, 0, LV_SYMBOL_STOP " Stopped");
  info(NOW, 0, "");
  info(TEXT, 0, "");
}

void setBufMeter(int val) {
  if (bufLvlMeter) lv_meter_set_indicator_end_value(bufLvlMeter, bufLvlIndicator, val);
}

void setTimeBar(int pct) {
  if (progTimeBar) lv_bar_set_value(progTimeBar, pct, LV_ANIM_OFF);
}

//Update buffer widget
void printBufStat(bool wrIsRunning, int wrCodec, int wrBitrate) {
    char buf[64] = "";
    const char * type = codecString[wrCodec];
    if (!wrIsRunning) snprintf(buf, 63, LV_SYMBOL_DOWNLOAD "\n%dk\n%s", wrBitrate / 1024, type);
    else snprintf(buf, 63, LV_SYMBOL_PLAY "\n%dk\n%s", wrBitrate / 1024, type);
    if (bufStatLbl && bufLvlMeter) {
      lv_label_set_text(bufStatLbl, buf);  //Set the text
      lv_obj_align_to(bufStatLbl, bufLvlMeter, LV_ALIGN_CENTER, 0, 4);  //Align below the label
    }
}

//Clear buffer widget
void clearBufStat() {
  if (bufLvlMeter) lv_meter_set_indicator_end_value(bufLvlMeter, bufLvlIndicator, 0);    
  if (bufStatLbl) lv_label_set_text(bufStatLbl, "");  //Set the text
}

//Compose Digital FM frequency display
void updateFMDisplay(int freq) {
  char str[15];
  if (freq < 30000) sprintf(str, "%d", freq);
  else if (freq < 100000) sprintf(str, " %.1f", (float)freq / 1000);
  else sprintf(str, "%.1f", (float)freq / 1000);
  if (fmFreqLbl) lv_label_set_text(fmFreqLbl, str);      
  if (progNowLbl) lv_label_set_text(progNowLbl, "");      
  if (progTextLbl) lv_label_set_text(progTextLbl, "");      
}

//Show an image in the DAB+ Icon area
void setDabImg(const void* src) {
  if (dab_img) lv_img_set_src(dab_img, src);
}

//Set the DAB+ image download progress bar
void setDabBar(int val, lv_anim_enable_t anim) {
  if (dab_img_bar) lv_bar_set_value(dab_img_bar, val, anim);
}

//Digital FM seek button actions
static void seekDownAction(lv_event_t * event) {
#ifdef MONKEYBOARD
  dabFMSearch(0);
#endif
#ifdef NXP6686
  NXPSearch(0);
#endif
}

static void seekUpAction(lv_event_t * event) {
#ifdef MONKEYBOARD
  dabFMSearch(1);
#endif
#ifdef NXP6686
  NXPSearch(1);
#endif
}

void setSignalMeter(int value) {
  lv_meter_set_indicator_value(signalMeter, signalIndic, value);
}

//---------------------------------------------
//Info label handler

#define PROG_LABEL_SIZE 1024
char* progBuffer = NULL;
char* progNameBuffer;
char* progNowBuffer;
char* progTextBuffer;
unsigned long progNameTimer = 0;
unsigned long progNowTimer = 0;
unsigned long progTextTimer = 0;

//Called from setup()
void setupInfoLabels() {
  progBuffer = (char*)ps_malloc(PROG_LABEL_SIZE);
  progNameBuffer = (char*)ps_malloc(PROG_LABEL_SIZE);
  progNowBuffer = (char*)ps_malloc(PROG_LABEL_SIZE);
  progTextBuffer = (char*)ps_malloc(PROG_LABEL_SIZE);
}

//Set an info label (NAME, NOW, TEXT) for a time or permanently if time is 0
void info(uint8_t field, int time, const char * format, ...) {
  char* buffer;
  lv_obj_t* label;
  unsigned long* timer;
  va_list args;
  //Ensure all initialised
  if (format == NULL || progBuffer == NULL || progNameLbl == NULL) return;
  //Setup pointers
  if (field == NAME) {
    buffer = progNameBuffer;
    label = progNameLbl;
    timer = &progNameTimer;
  } else if (field == NOW) {
    buffer = progNowBuffer;
    label = progNowLbl;
    timer = &progNowTimer;
  } else if (field == TEXT) {
    buffer = progTextBuffer;
    label = progTextLbl;
    timer = &progTextTimer;
  } else return;
  //variadic print to buffer and set label if it's time
  va_start (args, format);
  if (time) {
    vsnprintf(progBuffer, PROG_LABEL_SIZE-1, format, args);
    lv_label_set_text(label, progBuffer);
    *timer = millis() + (time * 1000);
  } else {
    vsnprintf (buffer, PROG_LABEL_SIZE-1, format, args);
    if (*timer == 0) lv_label_set_text(label, buffer);
  }
  va_end (args);
}

//Called from loop()
void infoLabelHandle() {
  //Purge any timed messages
  if (progNameTimer && progNameTimer < millis()) {
    progNameTimer = 0;
    lv_label_set_text(progNameLbl, progNameBuffer);
  }
  if (progNowTimer && progNowTimer < millis()) {
    progNowTimer = 0;
    lv_label_set_text(progNowLbl, progNowBuffer);
  }
  if (progTextTimer && progTextTimer < millis()) {
    progTextTimer = 0;
    lv_label_set_text(progTextLbl, progTextBuffer);
  }
}

//------------------------------------------------------------------------------------
// Popup window - action required

static lv_obj_t * popupWindow;

static void popupWindow_action(lv_event_t * event);

//Construct the popup and pop it up
void createPopupWindow(lv_obj_t * page, int y, char* prompt, void(*okFunction)(void), bool animated) {
  if (popupWindow) {
    lv_obj_del(popupWindow);
    animated = false;
  }
  popupWindow = lv_win_create(page, 45);
  lv_obj_t * popupHeaderLabel = lv_win_add_title(popupWindow, prompt);
  lv_obj_add_style(popupHeaderLabel, &style_biggerfont, LV_PART_MAIN);
#if (TFT_WIDTH == 480)
  lv_obj_set_size(popupWindow, 460, 45);
#else  
  lv_obj_set_size(popupWindow, 308, 45);
#endif
  lv_obj_set_pos(popupWindow, 4, y);
  lv_obj_add_style(popupWindow, &style_win, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(popupWindow, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t * header = lv_win_get_header(popupWindow);
  lv_obj_add_style(header, &style_groupbox, LV_PART_MAIN);

  lv_obj_t * ok_btn = lv_win_add_btn(popupWindow, LV_SYMBOL_OK, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(ok_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(ok_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(ok_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(ok_btn, popupWindow_action, LV_EVENT_CLICKED, (void*)okFunction);

  lv_obj_t * cancel_btn = lv_win_add_btn(popupWindow, LV_SYMBOL_CLOSE, 50);           /*Add close button and use built-in close action*/
  lv_obj_add_style(cancel_btn, &style_wp, LV_PART_MAIN);  
  lv_obj_add_style(cancel_btn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(cancel_btn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_add_event_cb(cancel_btn, popupWindow_action, LV_EVENT_CLICKED, NULL);
  if (animated) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, popupWindow);
    lv_anim_set_values(&a, LV_VER_RES, y);
#pragma GCC diagnostic push                         //Trouble at mill - pay no heed
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_time(&a, 500);
    lv_anim_start(&a);
  }
}

void popup(char* prompt, void(*okFunction)(void), bool animated) {
#ifdef VUMETER
  createPopupWindow(mainWindow, 112, prompt, okFunction, animated); 
#else
  createPopupWindow(mainWindow, 86, prompt, okFunction, animated); 
#endif
}

static void popupWindow_action(lv_event_t * event) {
  void (*okFunction)(void) = (void(*)())lv_event_get_user_data(event);
  if (okFunction) (*okFunction)();
  closePopup(true);
}

void popupHiddenAction(lv_anim_t * a) {
  lv_obj_del(popupWindow);
  popupWindow = NULL;  
}

void closePopup(bool animated) {
  if (popupWindow == NULL) return;
  if (animated) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, popupWindow);
    lv_anim_set_values(&a, lv_obj_get_y(popupWindow), LV_VER_RES);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
#pragma GCC diagnostic pop
    lv_anim_set_ready_cb(&a, popupHiddenAction);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_time(&a, 500);
    lv_anim_start(&a);
  } else {
    lv_obj_del(popupWindow);
    popupWindow = NULL;  
  }
}



