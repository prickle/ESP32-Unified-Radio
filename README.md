ESP32 Unified Radio

Full function webradio for ESP32 or ESP32S3

PlatformIO Project

Note that the included libraries have been modified for this project.

Features:

-- Webradio player
- Supports all stations, bitrates and codecs (so far)
- Webradio favourites list with full edit controls
- Webradio station search
  
-- Podcast player
- Episode lists
- Podcast favourites list with latest episode previews
- Podcast search

-- SD music file player
- File browser
- Playlist construction
- Playlist editor
- Support for album art  
- Image viewer

-- Windows media client
- File browser
- Playlists
- Album art
  
-- Ftp streaming client
- File browser
- Playlists
- Album art

-- Radio Receiver (using NXP6686)
- AM, FM, SW, LW
- Graphical band Scanner
- RDS Support

-- DAB+ Receiver (using Monkeyboard)
- Station slideshow display
- Also receives FM
- RDS Support
- Requires access to Monkeyboard library, contact me

-- Presets
- Programmable preset buttons
- Instant access to favourite stations, podcasts or broadcasts

-- Real time clock (updated from NTP, DAB or RDS)
- Weather forecast
- Current conditions
- 4 day forecast

-- Audio
- VU meter
- Spectrum Analyzer
- 3-band Equalizer
- Stereo Wide

-- Debugging
- Log viewer
- Performance monitor
- Factory functions

-- Extra
- Battery charge monitor
- Several different external button control methods


Example hardware target:

The "env:waveshare28s3" environment targets the Waveshare ESP32-S3-Touch-LCD-2.8, an ESP32S3 module with stereo speakers that works perfectly out of the box.

The "env:woofer" environment targets the WT32-SC01 Plus, an easily obtainable ESP32S3 module that works well with an external DAC or with it's single speaker in mono.

The "env:panasonic" environment targets the WT32-SC01, the plain ESP32 version using an external DAC.

The other environments use custom built hardware, profiles can be found near the top of defs.h

API Keys:

Copy the file data/keys.db.example to data/keys.db and fill in the fields. 

Visit api.openweathermap.org and create an account to get an openweathermap API key.
Set WEATHER_LOCATION to your openweathermap location.
Set WEATHER_OWMKEY to your API key.

Visit api.podcastindex.org and click Sign Up! to create an account and get a podcastindex API key.
Set PODCAST_KEY to your API key.
Set PODCAST_SECRET to your API secret.

First Run:

After first compiling and uploading the code, the device will pause during startup to format it's internal storage. This may take a few moments. When complete and the device is fully started up, upload the filesystem image with valid keys to complete the configuration. 

If you do use it, please credit me. Cheers!
