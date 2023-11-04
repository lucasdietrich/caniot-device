# Outdoor Alarm controller

Based on [BSP V1](bsp-tiny.md) board, with `ATmega328P` MCU.

## CANIOT

### Identification
- Class: `0`
- Device ID: `3`
- Name: `OutdoorAlarmController`
- Magic: `0x18a52468`

### Endpoints

- Application Level Control (0)
- Board Level Control (3)

## BSP

### Devices

- `TCN75` with I2C address `0x49` (`A2A1A0 = b001`)
- Unique OW `DS18B20` with address `0x28 0xcb 0x67 0x1b 0x33 0x20 0x01 0x30`

### IO Bindings

| Pin Index | Name | Description            | Binding                        | Behavior                               |
| --------- | ---- | ---------------------- | ------------------------------ | -------------------------------------- |
| 0         | OC1  | Open Collector 1 (out) | Outdoor light 1                | Set on = 1, Set off = 0                |
| 1         | OC2  | Open Collector 2 (out) | Outdoor light 2                | Set on = 1, Set off = 0                |
| 2         | RL1  | Relay 1       (out)    | Siren                          | Set on = 1, Set off = 0                |
| 3         | RL2  | Relay 2       (out)    | -                              | -                                      |
| 4         | IN1  | Input 1       (in)     | East presence sensor           | Presence detected = 1, No presence = 0 |
| 5         | IN2  | Input 2       (in)     | South presence sensor          | Presence detected = 1, No presence = 0 |
| 6         | IN3  | Input 3       (in)     | Sabotage south presence sensor | Compromised = 1, OK = 0                |
| 7         | IN4  | Input 4       (in)     | Sabotage east presence sensor  | Compromised = 1, OK = 0                |