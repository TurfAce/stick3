# Protocol references

The BLE GATT layout, characteristic constants, little-endian identity values,
and button frame format are based on the published work of Patrick Chwalek:
https://github.com/pchwalek/insta360_ble_esp32

The ORBIT identity advertisement layout was also checked against the X4 example:
https://github.com/btittelbach/esphome_config_examples/blob/main/insta360_ble_remote_waveshare_touch169_esp32s3.yaml

The application, button handling, connection gating, display, bounded receive
logging and host tests were written for this project. The referenced projects
do not establish compatibility with Insta360 X4 Air.

The MIT notice from insta360_ble_esp32 is retained below. M5Unified, M5GFX and
Arduino-ESP32 are fetched as dependencies and retain their upstream license
notices; their sources are not bundled in this archive.

## insta360_ble_esp32

MIT License

Copyright (c) 2026 pchwalek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


## esphome_config_examples (reference implementation)

Source: https://github.com/btittelbach/esphome_config_examples

The ORBIT identity advertising layout was checked against this project. Its
MIT notice is preserved below as a conservative attribution measure.

MIT License

Copyright (c) 2022 Bernhard Tittelbach

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.



## Build dependencies (downloaded separately)

| Component | Version | License / upstream notice |
|---|---|---|
| M5Unified | 0.2.29 | MIT; Copyright (c) 2021 M5Stack |
| M5GFX | 0.2.36 | MIT at project root, plus separately licensed bundled components |
| Arduino-ESP32 core | 2.0.17 | LGPL-2.1-or-later, subject to file-specific notices |
| ESP32 BLE Arduino (within Arduino-ESP32) | bundled in 2.0.17 | Apache-2.0; Copyright 2017 Neil Kolban |
| LovyanGFX-derived graphics code inside M5GFX | bundled | FreeBSD/BSD-style notices in source headers and upstream license.txt |
| Classic GLCD font (Font0) inside M5GFX | bundled | BSD-style; Copyright (c) 2012 Adafruit Industries |

These dependencies are not vendored in this source-only project. Downloading
them preserves their own notices; this table does not relicense them. A firmware
binary or preloaded-device distribution requires additional review of all
linked components, source/relinking requirements and bundled notices. This
file is not a complete binary-distribution compliance package.
