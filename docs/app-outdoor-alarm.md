# Outdoor Alarm controller

Based on BSP V1 board.

## CANIOT

### Identification
- Class: 0
- Device ID: 3
- Name: OutdoorAlarmController
- Magic: `0x18a52468`

### Endpoints

- Application Level Control (0)
- Board Level Control (3)

## BSP Bindings

| Pin Index | Name | Description            | Binding                        |
| --------- | ---- | ---------------------- | ------------------------------ |
| 0         | OC1  | Open Collector 1 (out) | Outdoor light 1                |
| 1         | OC2  | Open Collector 2 (out) | Outdoor light 2                |
| 2         | RL1  | Relay 1       (out)    | Siren                          |
| 3         | RL2  | Relay 2       (out)    | -                              |
| 4         | IN1  | Input 1       (in)     | East presence sensor           |
| 5         | IN2  | Input 2       (in)     | South presence sensor          |
| 6         | IN3  | Input 3       (in)     | Sabotage south presence sensor |
| 7         | IN4  | Input 4       (in)     | Sabotage east presence sensor  |