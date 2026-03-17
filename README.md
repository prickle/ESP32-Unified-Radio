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
The "env:woofer" environment targets the WT32-SC01 Plus, an easily obtainable ESP32S3 module that works well.
The "env:panasonic" environment targets the WT32-SC01, the plain ESP32 version.
The other environments use custom built hardware, profiles can be found near the top of defs.h

If you do use it, please credit me and get your own API keys. Cheers!
