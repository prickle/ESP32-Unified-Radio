#include "decls.h"

#ifdef SCANNER

lv_obj_t* receiverContainer;
lv_obj_t * scanChart;
lv_chart_series_t * scanChartSig;
lv_obj_t * scanBtn;
lv_obj_t * scanSlider;
lv_obj_t * blankerSlider;
lv_obj_t * scanBWDropdown;
lv_obj_t * scanAGCDropdown;
lv_chart_cursor_t* scanChartCursor = NULL;

int scanChartMin;
int scanChartMax;
int scanChartCursorPoint;

int* scanChartTable;

static void scanChart_event(lv_event_t * event);
static void scanSlider_event(lv_event_t * event);
static void blankerSlider_event(lv_event_t * event);
static void scan_action(lv_event_t * event);
static void scanDropdown_activated(lv_event_t * event);
static void scanBWDropdown_event(lv_event_t * event);
static void scanAGCDropdown_event(lv_event_t * event);

lv_obj_t* createReceiverWidget(lv_obj_t* parent) {

  static lv_style_t style_slider;
  lv_style_init(&style_slider);
#ifdef THEME_BLUE
  lv_style_set_border_color(&style_slider, lv_color_hex(0x487fb7));
#else
  lv_style_set_border_color(&style_slider, lv_color_hex(0xFF6025));
#endif
  lv_style_set_border_width(&style_slider, 2);
  lv_style_set_border_opa(&style_slider, LV_OPA_COVER);
  lv_style_set_radius(&style_slider, 4);
  lv_style_set_pad_all(&style_slider, 6);
  static lv_style_t style_sliderbg;
  lv_style_init(&style_sliderbg);
  lv_style_set_radius(&style_sliderbg, 4);
  static lv_style_t style_knob;
  lv_style_init(&style_knob);
  lv_style_set_radius(&style_knob, 4);
  lv_style_set_pad_all(&style_knob, 0);

  receiverContainer = lv_obj_create(parent);
  lv_obj_add_style(receiverContainer, &style_groupbox, LV_PART_MAIN);
  lv_obj_clear_flag(receiverContainer, LV_OBJ_FLAG_SCROLLABLE);
#if (TFT_WIDTH == 480)
  lv_obj_set_size(receiverContainer, 460, 180);
#else
  lv_obj_set_size(receiverContainer, 312, 180);
#endif
  scanSlider = lv_slider_create(receiverContainer);
  lv_obj_add_style(scanSlider, &style_slider, LV_PART_MAIN);
  lv_obj_add_style(scanSlider, &style_sliderbg, LV_PART_INDICATOR);
  lv_obj_add_style(scanSlider, &style_knob, LV_PART_KNOB);
  lv_slider_set_range(scanSlider, 1, 10);
  lv_obj_add_event_cb(scanSlider, scanSlider_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_size(scanSlider, 14, 90);
  lv_obj_set_pos(scanSlider, 4, 4);
  scanChart = lv_chart_create(receiverContainer);
  lv_obj_set_size(scanChart, 260, 75);
  lv_obj_align_to(scanChart, scanSlider, LV_ALIGN_OUT_RIGHT_TOP, 14, 0);  
  lv_obj_add_event_cb(scanChart, scanChart_event, LV_EVENT_ALL, NULL);
  lv_obj_refresh_ext_draw_size(scanChart);
  lv_chart_set_update_mode(scanChart, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_chart_set_div_line_count(scanChart, 5, 15);
  scanBtn = lv_btn_create(receiverContainer);
  lv_obj_set_size(scanBtn, 90, 25);
  lv_obj_add_style(scanBtn, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(scanBtn, &style_bigfont_orange, LV_PART_MAIN);
  lv_obj_add_style(scanBtn, &style_bigfont_orange, LV_PART_SELECTED);
  lv_obj_align_to(scanBtn, scanSlider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);
  lv_obj_add_event_cb(scanBtn, scan_action, LV_EVENT_CLICKED, NULL);
  lv_obj_t* label = lv_label_create(scanBtn);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(label, LV_SYMBOL_LOOP " Scan");
  scanBWDropdown = lv_dropdown_create(receiverContainer);
  lv_obj_set_size(scanBWDropdown, 90, 25);
  lv_obj_add_style(scanBWDropdown, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(scanBWDropdown, &style_biggerfont, LV_PART_MAIN);
#ifdef THEME_BLUE
  lv_obj_set_style_text_color(scanBWDropdown, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_MAIN);
#else
  lv_obj_set_style_text_color(scanBWDropdown, lv_color_hex(0xff1010), LV_PART_MAIN);
#endif
  lv_obj_align_to(scanBWDropdown, scanBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_add_event_cb(scanBWDropdown, scanBWDropdown_event, LV_EVENT_VALUE_CHANGED, NULL);                         //Set function to call on new option is chosen
  lv_obj_add_event_cb(scanBWDropdown, scanDropdown_activated, LV_EVENT_CLICKED, NULL);
  scanAGCDropdown = lv_dropdown_create(receiverContainer);
  lv_obj_set_size(scanAGCDropdown, 90, 25);
  lv_obj_add_style(scanAGCDropdown, &style_wp, LV_PART_MAIN);
  lv_obj_add_style(scanAGCDropdown, &style_biggerfont, LV_PART_MAIN);
#ifdef THEME_BLUE
  lv_obj_set_style_text_color(scanAGCDropdown, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_MAIN);
#else
  lv_obj_set_style_text_color(scanAGCDropdown, lv_color_hex(0xff1010), LV_PART_MAIN);
#endif
  lv_obj_align_to(scanAGCDropdown, scanBWDropdown, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_add_event_cb(scanAGCDropdown, scanAGCDropdown_event, LV_EVENT_VALUE_CHANGED, NULL);                         //Set function to call on new option is chosen
  lv_obj_add_event_cb(scanAGCDropdown, scanDropdown_activated, LV_EVENT_CLICKED, NULL);

  blankerSlider = lv_slider_create(receiverContainer);
  lv_obj_add_style(blankerSlider, &style_slider, LV_PART_MAIN);
  lv_obj_add_style(blankerSlider, &style_sliderbg, LV_PART_INDICATOR);
  lv_obj_add_style(blankerSlider, &style_knob, LV_PART_KNOB);
  lv_slider_set_range(blankerSlider, 1, 10);
  lv_obj_add_event_cb(blankerSlider, blankerSlider_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_size(blankerSlider, 100, 14);
  lv_obj_align_to(blankerSlider, scanBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

  return receiverContainer;  
}

static void scanDropdown_activated(lv_event_t * event) {
  lv_obj_t * dropdown = lv_event_get_target(event);
  lv_obj_t * ddlist = lv_dropdown_get_list(dropdown);
  lv_state_t state = lv_obj_get_state(dropdown);
  if (ddlist && (state & LV_STATE_CHECKED)) {  
    lv_obj_add_style(ddlist, &style_biggerfont, LV_PART_MAIN);
    lv_obj_set_style_max_height(ddlist, 300, LV_PART_MAIN);
    lv_obj_set_height(ddlist, LV_SIZE_CONTENT);
  }
}

static void scanBWDropdown_event(lv_event_t * event) {
  uint16_t opt = lv_dropdown_get_selected(lv_event_get_target(event));      //Get the id of selected option
  NXPSetBandwidth(opt);
  if (settings->mode == MODE_NFM) settings->bwFM = opt;
  else if (settings->mode == MODE_NLW) settings->bwLW = opt;
  else if (settings->mode == MODE_NMW) settings->bwMW = opt;
  else if (settings->mode == MODE_NSW) settings->bwSW = opt;
  writeSettings();
}

static void scanAGCDropdown_event(lv_event_t * event) {
  uint16_t opt = lv_dropdown_get_selected(lv_event_get_target(event));      //Get the id of selected option
  NXPSetAGC(opt);
  if (settings->mode == MODE_NFM) settings->agcFM = opt;
  else if (settings->mode == MODE_NLW) settings->agcLW = opt;
  else if (settings->mode == MODE_NMW) settings->agcMW = opt;
  else if (settings->mode == MODE_NSW) settings->agcSW = opt;
  writeSettings();
}

static void scanSlider_event(lv_event_t * e)
{
  lv_obj_t * obj = lv_event_get_target(e);
  int32_t v = lv_slider_get_value(obj);
  lv_point_t offset = {0, 0};
  if (scanChartCursor) lv_chart_get_point_pos_by_id(scanChart, scanChartSig, scanChartCursorPoint, &offset);
  lv_chart_set_zoom_x(scanChart, v * LV_IMG_ZOOM_NONE);
  if(scanChartCursor) {
    lv_point_t p;
    lv_chart_get_point_pos_by_id(scanChart, scanChartSig, scanChartCursorPoint, &p);
    p.x += lv_obj_get_scroll_x(scanChart) - offset.x;
    lv_obj_scroll_to_x(scanChart, p.x, LV_ANIM_OFF);
  }
}


static void scanChart_event(lv_event_t * event) {
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t * chart = lv_event_get_target(event);
  static int32_t id = LV_CHART_POINT_NONE;

  if(code == LV_EVENT_VALUE_CHANGED) {
      lv_obj_invalidate(chart);
  }
  else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
      lv_coord_t * s = (lv_coord_t*)lv_event_get_param(event);
      *s = LV_MAX(*s, 20);
  }
  else if(code == LV_EVENT_DRAW_PART_BEGIN) {
    lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t*)lv_event_get_param(event);
    if(dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X) {
      float step = (scanChartTable[NXPTABLE_TMAX] - scanChartTable[NXPTABLE_TMIN]) / 9.0;
      lv_snprintf(dsc->text, sizeof(dsc->text), "%d", (int)(scanChartTable[NXPTABLE_TMIN] + (step * dsc->value)));
    }
  }
  else if(code == LV_EVENT_PRESSED) {
    id = lv_chart_get_pressed_point(chart);
  }
  else if(code == LV_EVENT_SCROLL) {
    id = LV_CHART_POINT_NONE;
  }
  else if(code == LV_EVENT_RELEASED) {
    lv_obj_invalidate(chart);
    if(id != LV_CHART_POINT_NONE) {
      int min = scanChartTable[NXPTABLE_FMIN];
      int step = scanChartTable[NXPTABLE_STEP];
      int value = min + (id * step);
      NXPSetFrequency(value);
      id = LV_CHART_POINT_NONE;
    }
  }
}

void scan_action(lv_event_t* event) {
  NXPScan();
}


void scanBWOptions(const char* options, int selected) {
  lv_dropdown_set_options(scanBWDropdown, options);
  lv_dropdown_set_selected(scanBWDropdown, selected);
}

void scanAGCOptions(const char* options) {
  lv_dropdown_set_options(scanAGCDropdown, options);
}
void scanAGCSelected(int selected) {
  lv_dropdown_set_selected(scanAGCDropdown, selected);
}

void scanAdd(int frequency, int value) {
  if (!scanChart || !scanChartSig) return;
  int point = (frequency - scanChartTable[NXPTABLE_FMIN]) / scanChartTable[NXPTABLE_STEP]; 
  lv_chart_set_value_by_id(scanChart, scanChartSig, point, value);
  if (value < scanChartMin) {
    scanChartMin = value;
    lv_chart_set_range(scanChart, LV_CHART_AXIS_PRIMARY_Y, scanChartMin, scanChartMax);
  }
  if (value > scanChartMax) {
    scanChartMax = value;
    lv_chart_set_range(scanChart, LV_CHART_AXIS_PRIMARY_Y, scanChartMin, scanChartMax);
  }
  lv_chart_refresh(scanChart);
}

void scanCursor(int frequency) {
  if (!scanChart || !scanChartSig) return;
  if (!scanChartCursor) scanChartCursor = lv_chart_add_cursor(scanChart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_VER);  
  if (frequency < 0) {
    lv_point_t p = {(lv_coord_t)frequency, 0};
    if (scanChartCursor) lv_chart_set_cursor_pos(scanChart, scanChartCursor, &p);
  }
  else {
    scanChartCursorPoint = (frequency - scanChartTable[NXPTABLE_FMIN]) / scanChartTable[NXPTABLE_STEP];
    lv_chart_set_cursor_point(scanChart, scanChartCursor, scanChartSig, scanChartCursorPoint);
    lv_point_t screenPos;
    lv_chart_get_point_pos_by_id(scanChart, scanChartSig, scanChartCursorPoint, &screenPos);
    if (screenPos.x < 0) { 
      screenPos.x += lv_obj_get_scroll_x(scanChart);
      lv_obj_scroll_to_x(scanChart, screenPos.x, LV_ANIM_OFF);
    }
    else if(screenPos.x > (lv_obj_get_width(scanChart)-3)) {
      screenPos.x += lv_obj_get_scroll_x(scanChart) - (lv_obj_get_width(scanChart)-3);
      lv_obj_scroll_to_x(scanChart, screenPos.x, LV_ANIM_OFF);
    }
  }
}

void scanClear(int* freqTable) {
  if (!scanChart) return;
  if (scanChartSig) lv_chart_remove_series(scanChart, scanChartSig);
  scanChartSig = NULL;
  scanCursor(-100);
  scanChartTable = freqTable;
  lv_chart_set_zoom_x(scanChart, LV_IMG_ZOOM_NONE);
  lv_slider_set_value(scanSlider, 1, LV_ANIM_OFF);
  int points = (freqTable[NXPTABLE_FMAX] - freqTable[NXPTABLE_FMIN]) / freqTable[NXPTABLE_STEP] + 1;
  if (points) {
    scanChartSig = lv_chart_add_series(scanChart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    scanChartMin = freqTable[NXPTABLE_SMIN];
    scanChartMax = freqTable[NXPTABLE_SMAX];
    lv_chart_set_range(scanChart, LV_CHART_AXIS_PRIMARY_Y, scanChartMin, scanChartMax);
    lv_chart_set_axis_tick(scanChart, LV_CHART_AXIS_PRIMARY_X, 6, 4, 10, 5, true, 30);
    lv_chart_set_point_count(scanChart, points);
  }
}

/*void scanClear(int points, int start, int end, int min, int max) {
  if (!scanChart) return;
  if (scanChartSig) lv_chart_remove_series(scanChart, scanChartSig);
  if (scanChartFrq) lv_chart_remove_series(scanChart, scanChartFrq);
  scanCursor(-100);
  lv_chart_set_zoom_x(scanChart, LV_IMG_ZOOM_NONE);
  if (points) {
    scanChartSig = lv_chart_add_series(scanChart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    scanChartFrq = lv_chart_add_series(scanChart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_hide_series(scanChart, scanChartFrq, true);
    scanChartMin = min;
    scanChartMax = max;
    scanChartStart = start;
    scanChartEnd = end;
    lv_chart_set_range(scanChart, LV_CHART_AXIS_PRIMARY_Y, min, max);
    lv_chart_set_axis_tick(scanChart, LV_CHART_AXIS_PRIMARY_X, 6, 4, 10, 5, true, 30);
    lv_chart_set_point_count(scanChart, points);
  }
}*/

static void blankerSlider_event(lv_event_t * e)
{
  lv_obj_t * obj = lv_event_get_target(e);
  int32_t v = lv_slider_get_value(obj);
  NXPSetNoiseBlanker(v);
}

#endif

