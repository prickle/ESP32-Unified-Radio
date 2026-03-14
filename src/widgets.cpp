#include "decls.h"
//-------------------------------------------------
// Widgets

//Place a Adafruit canvas into an LVGL image
// Allows us to draw LVGL widgets using Adafruit drawing commands
void fillImageDescription(lv_img_dsc_t* desc, GFXcanvas16* canvas) {
  desc->header.cf = LV_IMG_CF_TRUE_COLOR,          //Set the color format
  desc->header.reserved = 0,
  desc->header.always_zero = 0,
  desc->header.w = canvas->width(); 
  desc->header.h = canvas->height(); 
  desc->data_size = canvas->width() * canvas->height() * LV_COLOR_DEPTH / 8; 
  desc->data = (uint8_t*)canvas->getBuffer();   
}

//------------------------------------------------------
//VU meter widget

#ifdef VUMETER

//Static canvas for VU
GFXcanvas16* vuBuffer = 0;
static lv_img_dsc_t vu_img_dsc;
int vu_prev_x[2], vu_peak_x[2];
int16_t lvuVal, rvuVal;
unsigned long vuTimer;
uint16_t vuBackCol;

lv_obj_t* createVU(lv_obj_t* parent, int w, int h, uint16_t backCol) {
  lv_obj_t *img = lv_img_create(parent);
  uint16_t* buf = (uint16_t*)ps_malloc(w * h * 2);
  vuBuffer = new GFXcanvas16(w, h, buf);
  vuBuffer->fillRect(0, 0, w, h, (vuBackCol = backCol));
  fillImageDescription(&vu_img_dsc, vuBuffer);
  return img;
}

//Draw stereo VU level meter
//Called from audio
// just updates values, handled from loop
void drawVU(int16_t l, int16_t r) {
  if (l > lvuVal) lvuVal = l;
  if (r > rvuVal) rvuVal = r;
}

//VU meter handled from loop()
void VUHandle() {
  if (!vuBuffer) return;
#ifdef MONKEYBOARD
  if (settings->mode == MODE_DAB || settings->mode == MODE_FM) return;
#endif  
  if (millis() < vuTimer) return;
  vuTimer = millis() + 20;
  for (size_t i = 0; i < 2; ++i) {
    int32_t x = ((i ? rvuVal : lvuVal) * (vuBuffer->width() - 2)) / INT16_MAX;
    int32_t px = vu_prev_x[i];
    if (px - 5 >= x) x = px - 5;
    if (px != x) {
      if (px < x) vuBuffer->fillRect(px, i * 4, x - px, 2, DKGREEN);
      else vuBuffer->fillRect(x, i * 4, px - x, 2, DKRED);
      vu_prev_x[i] = x;
    }
    px = vu_peak_x[i];
    if (px > x) {
      vuBuffer->drawFastVLine(px, i * 4, 2, vuBackCol);
      vuBuffer->drawFastVLine(px+1, i * 4, 2, vuBackCol);
      px-=1;
    }
    if (px < x) px = x;
    if (vu_peak_x[i] != px) {
      vu_peak_x[i] = px;
      vuBuffer->drawFastVLine(px, i * 4, 2, WHITE);
      vuBuffer->drawFastVLine(px+1, i * 4, 2, WHITE);
    }
  }
  lvuVal = 0;
  rvuVal = 0;
  lv_img_set_src(vuMeter, &vu_img_dsc);
}

#endif
//--------------------------------------------------------------------
//Wind speed bar widget

//Keep the canvas with the images
typedef struct canvasImg {
  GFXcanvas16* img = 0;
  lv_img_dsc_t img_dsc;
} canvasImg;

lv_obj_t* createWindSpd(lv_obj_t* parent, int w, int h) {
  canvasImg* canvas = new canvasImg;
  lv_obj_t *img = lv_img_create(parent);
  canvas->img = new GFXcanvas16(w, h);
  fillImageDescription(&(canvas->img_dsc), canvas->img);
  lv_obj_set_user_data(img, canvas);
  return img;
}

void drawWindSpd(lv_obj_t* obj, float speed, float gust) {
  canvasImg* canvas = (canvasImg*)lv_obj_get_user_data(obj);
  if (!canvas) return;
  GFXcanvas16* windSpdBuffer = canvas->img;  
  if (!windSpdBuffer) return;
  int d = windSpdBuffer->width() - 1;
  int h = windSpdBuffer->height() - 1;
  if (gust > 10) gust = 10;
  if (speed > 10) {
    //Make bar all red because gust will be > 10
    speed = 0;
  }
  windSpdBuffer->fillRoundRect(0, 0, d, h, 4, WHITE);
  windSpdBuffer->fillRoundRect(1, 1, d-2, h-2, 4, BLACK);
  if (gust > 0.2) {
    float bar = (gust / 10) * (d - 4) + 1;
    windSpdBuffer->fillRoundRect(1, 1, bar, h-2, 4, LTRED);
  }
  if (speed > 0.2) {
    float bar = (speed / 10) * (d - 4) + 1;
    windSpdBuffer->fillRoundRect(1, 1, bar, h-2, 4, LTGREEN);
  }
  lv_img_set_src(obj, &(canvas->img_dsc));
}

//----------------------------------------------------
// FFT widget

#ifdef FFTMETER
uint16_t fftBackCol;

//FFT meter
typedef struct fftImg {
  GFXcanvas16* img = 0;
  lv_img_dsc_t img_dsc;
  uint8_t barWidth;
  uint8_t numBars;
} fftImg;

int fft_prev_y[64] = { 0 };
float fft_peak_y[64] = { 0 };

lv_obj_t* createFFT(lv_obj_t* parent, int w, int h, uint16_t backCol) {
  fftImg* fft = new fftImg;
  lv_obj_t *img = lv_img_create(parent);
  fft->img = new GFXcanvas16(w, h);
  fft->img->fillRect(0, 0, w, h, (fftBackCol = backCol));
  fft->barWidth = w / FFT_BARS;
  fft->numBars = w / fft->barWidth;
  if (fft->numBars > 64) fft->numBars = 64;
  fillImageDescription(&(fft->img_dsc), fft->img);
  lv_obj_set_user_data(img, fft);
  return img;
}

void drawFFT(lv_obj_t* obj, uint8_t* data) {
  fftImg* fft = (fftImg*)lv_obj_get_user_data(obj);
  if (!fft) return;
  //int16_t bgcol = lv_color_to16(lv_theme_get_color_primary(obj));
  GFXcanvas16* fftBuffer = fft->img;  
  if (!fftBuffer) return;
  int w = fftBuffer->width() - 1;
  int h = fftBuffer->height() - 1;
  for (int bar = 0; bar < fft->numBars; bar++) {
    int x = bar * fft->barWidth;
    int y = (data[bar] * (h - 1)) / 256;
    if (y > h) y = h;
    int py = fft_prev_y[bar];
    if (py - 2 >= y) y = py - 2;
    if (py != y) {
      if (py > y) fftBuffer->fillRect(x, h - py, fft->barWidth - 1, py - y, RED);
      else fftBuffer->fillRect(x, h - y, fft->barWidth - 1, y - py, LTGREEN);
      fft_prev_y[bar] = y;
    }
    float pk = fft_peak_y[bar];
    if (pk > y) {
      fftBuffer->drawFastHLine(x, h - pk - 1, fft->barWidth - 1, fftBackCol);
      pk-=0.5;
    }
    else pk = y;
    
    if (fft_peak_y[bar] != (int)pk) {
      fft_peak_y[bar] = (int)pk;
      if (pk > 1) fftBuffer->drawFastHLine(x, h - (int)pk - 1, fft->barWidth - 1, WHITE);
    }
  }
  lv_img_set_src(obj, &(fft->img_dsc));
}


//-------------------------------------------------------------
//FFT in PSRAM

//#define FFT_IN_PSRAM

#ifdef FFT_IN_PSRAM
float* _wr;
float* _wi;
float* _fr;
float* _fi;
uint16_t* _br;
#else
float _wr[FFT_SIZE + 1];
float _wi[FFT_SIZE + 1];
float _fr[FFT_SIZE + 1];
float _fi[FFT_SIZE + 1];
uint16_t _br[FFT_SIZE + 1];
#endif
size_t _ie;

void initFFT(void) {
#ifdef FFT_IN_PSRAM
  _wr = (float*)ps_malloc((FFT_SIZE + 1) * sizeof(float));
  _wi = (float*)ps_malloc((FFT_SIZE + 1) * sizeof(float));
  _fr = (float*)ps_malloc((FFT_SIZE + 1) * sizeof(float));
  _fi = (float*)ps_malloc((FFT_SIZE + 1) * sizeof(float));
  _br = (uint16_t*)ps_malloc((FFT_SIZE + 1) * sizeof(uint16_t));
#endif
  _ie = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
  static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
  static constexpr int s4 = FFT_SIZE / 4;
  static constexpr int s2 = FFT_SIZE / 2;
  for ( int i = 1 ; i < s4 ; ++i) {
    float f = cosf(omega * i);
    _wi[s4 + i] = f;
    _wi[s4 - i] = f;
    _wr[     i] = f;
    _wr[s2 - i] = -f;
  }
  _wi[s4] = _wr[0] = 1;

  size_t je = 1;
  _br[0] = 0;
  _br[1] = FFT_SIZE / 2;
  for ( size_t i = 0 ; i < _ie - 1 ; ++i ) {
    _br[ je << 1 ] = _br[ je ] >> 1;
    je = je << 1;
    for ( size_t j = 1 ; j < je ; ++j )
      _br[je + j] = _br[je] + _br[j];
  }
}


void execFFT(const int16_t* in) {
  memset(_fi, 0, (FFT_SIZE + 1) * sizeof(float));
  for ( size_t j = 0 ; j < FFT_SIZE / 2 ; ++j ) {
    float basej = 0.25 * (1.0-_wr[j]);
    size_t r = FFT_SIZE - j - 1;
    /// perform han window and stereo to mono convert.
    _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
    _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
  }

  size_t s = 1;
  size_t i = 0;
  do {
    size_t ke = s;
    s <<= 1;
    size_t je = FFT_SIZE / s;
    size_t j = 0;
    do {
      size_t k = 0;
      do {
        size_t l = s * j + k;
        size_t m = ke * (2 * j + 1) + k;
        size_t p = je * k;
        float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
        float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
        _fr[m] = _fr[l] - Wxmr;
        _fi[m] = _fi[l] - Wxmi;
        _fr[l] += Wxmr;
        _fi[l] += Wxmi;
      } while ( ++k < ke) ;
    } while ( ++j < je );
  } while ( ++i < _ie );
}


uint32_t getFFT(size_t index) {
  return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[ index ] * _fr[ index ] + _fi[ index ] * _fi[ index ]) : 0u;
}

#endif