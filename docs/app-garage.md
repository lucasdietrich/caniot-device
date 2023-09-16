# Garage door controller

Based on BSP V1 board.

## CANIOT

### Identification
- Class: 0
- Device ID: 1
- Name: GarageDoorController
- Magic: `0x55b0f0d7`

### Endpoints

- Board Level Control (3)

## BSP Bindings

| Pin Index | Name | Description            | Binding           |
| --------- | ---- | ---------------------- | ----------------- |
| 0         | OC1  | Open Collector 1 (out) | -                 |
| 1         | OC2  | Open Collector 2 (out) | -                 |
| 2         | RL1  | Relay 1       (out)    | Left door         |
| 3         | RL2  | Relay 2       (out)    | Right door        |
| 4         | IN1  | Input 1       (in)     | -                 |
| 5         | IN2  | Input 2       (in)     | Gate status       |
| 6         | IN3  | Input 3       (in)     | Left door status  |
| 7         | IN4  | Input 4       (in)     | Right door status |