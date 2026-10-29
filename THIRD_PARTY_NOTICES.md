# Third-party notices

## STM32 linker script

`firmware/stm32/STM32F103C8_APP.ld` derives from the STM32F103C8Tx linker script supplied with the ST/PlatformIO toolchain. The local change reserves the last 8 KiB of nominal flash for the application's two storage banks. Its original notice is retained in the file.

Copyright (c) 2021 STMicroelectronics. All rights reserved.

BSD 3-Clause License:

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Build-time dependencies

PlatformIO downloads the STM32Cube/CMSIS/HAL, Arduino ESP32/ESP-IDF and compiler packages specified by the pinned build environments. They are not vendored into this repository and retain their own licenses/notices. Review those packages' licensing when distributing firmware binaries.

## Original application code

No project-wide license grant has been selected for the original application code. Public visibility does not by itself grant an open-source redistribution license. The owner can choose and add a license separately without altering the third-party notices above.
