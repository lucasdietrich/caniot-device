# Heaters controller

Based on [BSP Tiny rev A](bsp-tiny.md) board, with `ATmega328P` MCU.

## CANIOT

### Identification
- Class: `1`
- Device ID: `0`
- Name: `HeatingController`
- Magic: `0x523a5a5b`

### Endpoints

- Application Level Control (0)
- Board Level Control (3)

### Application Level Control (0)

![pics/heating-controller-app-endpoint-0.png](pics/heating-controller-app-endpoint-0.png)

## BSP

### Devices

- `TCN75` with I2C address `0x49` (`A2A1A0 = b001`)
- `PCF8574A`
- Unique OW `DS18B20` with *unknown* address
- 4 Heaters

### IO Bindings

| Pin Index | Name | Description       | Binding                  | Behavior                |
| --------- | ---- | ----------------- | ------------------------ | ----------------------- |
| 0         | PC0  | MCU GPIO          | -                        | -                       |
| 1         | PC1  | MCU GPIO          | -                        | -                       |
| 2         | PC2  | MCU GPIO          | Grid Power Presence (in) | 1 = Present, 0 = Absent |
| 3         | PC3  | MCU GPIO          | -                        | -                       |
| 4         | PD4  | MCU GPIO          | -                        | -                       |
| 5         | PD5  | MCU GPIO          | -                        | -                       |
| 6         | PD6  | MCU GPIO          | -                        | -                       |
| 7         | PD7  | MCU GPIO          | -                        | -                       |
| 8         | EIO0 | EXTERNAL PCF GPIO | Heater 4 Neg OC (out 4L) | -                       |
| 9         | EIO1 | EXTERNAL PCF GPIO | Heater 4 Pos OC (out 4H) | -                       |
| 10        | EIO2 | EXTERNAL PCF GPIO | Heater 3 Neg OC (out 3L) | -                       |
| 11        | EIO3 | EXTERNAL PCF GPIO | Heater 3 Pos OC (out 3H) | -                       |
| 12        | EIO4 | EXTERNAL PCF GPIO | Heater 2 Neg OC (out 2L) | -                       |
| 13        | EIO5 | EXTERNAL PCF GPIO | Heater 2 Pos OC (out 2H) | -                       |
| 14        | EIO6 | EXTERNAL PCF GPIO | Heater 1 Neg OC (out 1L) | -                       |
| 15        | EIO7 | EXTERNAL PCF GPIO | Heater 1 Pos OC (out 1H) | -                       |
| 16        | PB0  | MCU GPIO          | -                        | -                       |
| 17        | PE0  | MCU GPIO          | -                        | -                       |
| 18        | PE1  | MCU GPIO          | -                        | -                       |