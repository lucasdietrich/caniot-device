# CANIOT 2 devices

## Devices :

- Garage Door Controller
- Alarm Controller

## Arch

- IDLE thread
- Main thread -> CANIOT
- CAN Thread
- Workqueue : Monitoring, measurements, tasks

## Todo

- Set pullup to inputs and INT0
- Define specific device configuration, add support for configuration in persistant memory
- Add documentation for data types
  - Use endpoints for specific uses (normal, configuration, ...)
- ~~Implement precision uptime~~
- Implement invalid value handling for telemetry (e.g. temperature, etc...)

## Monitor using screen (on linux)

```
screen /dev/ttyACM0 115200
```

Exit screen with shortcuts : `Ctrl + A` and `Ctrl + \` meaning (AltGr + 8), then `y`.

## 1. Garage Door Controller

## 2. Alarm Controller

- Turn lights on when alarm is triggered
- Don't turn lights on during daytime
- Don't send first telemetry before external temperature is available

Logs
```
ROM = 28 bc 49 9c 32 20 1 83: DS18B20
===== k_thread =====
C 0x045C READY   C ____ : SP 21/110:0x016D
A 0x0470 READY   C ____ : SP 21/160:0x020D
W 0x0484 READY   C ____ : SP 21/128:0x02D1
M 0x0498 READY   C ____ : SP 0/250:0x06D4
I 0x04AC READY   P ____ : SP 21/59:0x0248
name    = AlarmController
cls/dev = 0/3
version = c8

00:00:00 [0.026 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 00 f4 01 00 00 46 00 
[C] CANARIES until @0133 [found 50], MAX usage = 59 / 109 + 1 (sentinel) 
[A] CANARIES until @01EE [found 127], MAX usage = 32 / 159 + 1 (sentinel)
[W] CANARIES until @0293 [found 64], MAX usage = 63 / 127 + 1 (sentinel)
[M] CANARIES until @0620 [found 68], MAX usage = 181 / 249 + 1 (sentinel)
[I] CANARIES until @0220 [found 17], MAX usage = 41 / 58 + 1 (sentinel)
00:00:16 [16.450 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 10 00 00 00 00 00 00
00:00:16 [16.452 s] : alarm: inactive -> observing
00:00:16 [16.454 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 04 00 f4 01 00 00 7d 00
00:00:21 [21.571 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 01 00 00 00 00 00 00
00:00:21 [21.574 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 05 00 f4 01 00 00 7d 00
00:00:22 [22.430 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 04 00 00 00 00 00 00
00:00:22 [22.439 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 07 00 f4 01 00 00 7d 00
00:00:25 [25.043 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 00 00 00 00 00 00 00
00:00:25 [25.046 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 07 00 f4 01 00 00 7d 00
00:00:26 [26.184 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 00 00 00 00 00 00 00
00:00:26 [26.187 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 07 00 f4 01 00 00 7d 00
00:00:27 [27.013 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 00 00 00 00 00 00 00
00:00:27 [27.016 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 07 00 f4 01 00 00 7d 00
00:00:28 [28.133 s] : [ c0 ] Command Query [24] 0x18 (cls=C0 sid=D3) : ep-0 / 00 00 00 00 00 00 00 00
00:00:28 [28.136 s] : [ c5 ] Telemetry Response [24] 0x18 (cls=C0 sid=D3) : ep-0 / 07 00 f4 01 00 00 7d 00
[C] CANARIES until @0132 [found 49], MAX usage = 60 / 109 + 1 (sentinel)
[A] CANARIES until @0194 [found 37], MAX usage = 122 / 159 + 1 (sentinel)
[W] CANARIES until @0293 [found 64], MAX usage = 63 / 127 + 1 (sentinel)
[M] CANARIES until @061D [found 65], MAX usage = 184 / 249 + 1 (sentinel)
[I] CANARIES until @0220 [found 17], MAX usage = 41 / 58 + 1 (sentinel)
```

## Bootloader

- https://docs.arduino.cc/built-in-examples/arduino-isp/ArduinoISP
- https://arduino.stackexchange.com/questions/36071/how-does-avrdude-burn-a-bootloader-much-quicker-than-the-arduino-ide
- https://www.avrfreaks.net/forum/cannot-connect-avrdude-mega2560
- https://github.com/MCUdude/MiniCore
- https://github.com/MCUdude/MiniCore/blob/master/PlatformIO.md
- https://github.com/MCUdude/MiniCore/tree/master/avr/bootloaders/optiboot_flash/bootloaders/atmega328pb/16000000L
- https://community.platformio.org/t/upload-and-debug-for-atmega328p-with-atmel-ice/18796
- https://community.platformio.org/t/burn-bootloader-for-a-custom-atmega328p-board/22587
- https://community.platformio.org/t/minicore-atmega328p/9042
- https://docs.platformio.org/en/latest/frameworks/arduino.html#minicore-mightycore-megacore
- https://www.avrfreaks.net/forum/watchdog-reset-or-any-other-software-reset
- https://www.avrfreaks.net/forum/software-reset-6
- https://www.avrfreaks.net/forum/how-use-avr-watchdog

- https://www.avrfreaks.net/comment/178013#comment-178013