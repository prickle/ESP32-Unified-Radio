# ESP32 Unified Radio

Full function webradio, podcast player, MP3 player, music streamer, bluetooth speaker and broadcast radio receiver for ESP32 or ESP32S3

This radio is intended to be like an old-fashioned, stand-alone, hands on appliance. No external control or setup is required after initial installation, all functionality is available on the device itself. This makes it ideal for retrofitting into old radio or portable stereo chassies, restoring them to use or giving them more life.

This is a PlatformIO Project using ESP-IDF and Arduino, based on [LVGL](https://lvgl.io/), [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S), [LovyanGFX](https://github.com/lovyan03/LovyanGFX) and many other libraries.

Note that the included libraries have been modified for this project. All code remains the property of the respective authors.

## Features:

**Webradio player**
- Supports all stations, bitrates and codecs (so far)
- Webradio favourites list with full edit controls
- Webradio station search
  
**Podcast player**
- Episode lists
- Podcast favourites list with latest episode previews
- Podcast search

**SD music file player**
- File browser
- Playlist construction
- Playlist editor
- Support for album art  
- Image viewer

**Windows media client**
- File browser
- Playlists
- Album art
  
**Ftp streaming client**
- File browser
- Playlists
- Album art

**Radio Receiver (using NXP6686)**
- Uses NXP6686 I2S for DAC output
- AM, FM, SW, LW
- Graphical band Scanner
- RDS Support

**DAB+ Receiver (using Monkeyboard)**
- Uses Monkeyboard I2S for DAC output
- DAB+ Receiver, station scan and RDS support
- Full-screen station slideshow display (320x240)
- Also receives FM with RDS Support
- Requires access to Monkeyboard library, contact me

**Bluetooth**
- Only on ESP32 (not S3)
- Bluetooth audio receiver
- Remote volume control
- Metadata support

**Presets**
- Programmable preset buttons
- Instant access to favourite stations, podcasts or broadcasts

**Real time clock**
- Time updated from NTP, DAB or RDS
- Timezone and DST support

**Weather forecast**
- Current conditions
- 4 day forecast

**Audio**
- VU meter
- Spectrum Analyzer
- 3-band Equalizer
- Stereo Wide

**Debugging**
- Log viewer
- Performance monitor
- Factory functions

**Extra**
- Onscreen keyboard
- Battery charge monitor
- Several different external button control methods
- OTA programming option


## Example hardware target:

The "env:waveshare28s3" environment targets the [Waveshare ESP32-S3-Touch-LCD-2.8](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8), a compact ESP32S3 TFT touchscreen module with stereo speakers that works perfectly out of the box.

The "env:woofer" environment targets the [WT32-SC01 Plus](https://en.wireless-tag.com/product-item-26.html), an easily obtainable (and larger) ESP32S3 TFT touchscreen module that works well with an external DAC or with it's single speaker in mono.

The "env:panasonic" environment targets the [WT32-SC01](https://en.wireless-tag.com/product-item-25.html), a plain ESP32 TFT touchscreen module using an external DAC.

The other environments use custom built hardware, mostly based off ESP32-WROVER modules with at least 4M flash and 2M PSRAM. Profiles can be found near the top of defs.h.

## Installation:

Install Visual Studio Code and install the PlatformIO extension from within Visual Studio. Download the ESP32 Unified Radio source code from here and open it from PlatformIO. It will configure itself and download any required libraries. Select an environment to suit your hardware, prepare your API keys and you are ready to compile and upload to the board.

## API Keys:

Copy the file data/keys.db.example to data/keys.db and fill in the fields: 

Visit api.openweathermap.org and create an account to get an openweathermap API key.

Set WEATHER_LOCATION to your openweathermap location.

Set WEATHER_OWMKEY to your API key.

Visit api.podcastindex.org and click Sign Up! to create an account and get a podcastindex API key.

Set PODCAST_KEY to your API key.

Set PODCAST_SECRET to your API secret.

## First Run:

After first compiling and uploading the code, the device will pause during startup to format it's internal SPIFFS storage. This may take a few moments. When complete and the device is fully started up, upload the filesystem image with valid keys to complete the configuration. 

If you do use it, please credit me. Cheers!
