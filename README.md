# Vault Guard

**Vault Guard** is an embedded intrusion detection system built using the STM32 Nucleo-F446RE. It combines multiple sensors and RFID-based authentication to monitor and protect a secure vault environment.

## Overview

The system detects unauthorized access attempts using:
- Light Variation (LDR)
- Physical disturbances (Vibration Sensor)
- Unauthorized access (RFID authentication)

## Components Used

- STM32 Nucleo-F446RE - Microcontroller
- LM373 - LDR
- LM393 - Vibration Sensor
- PN532 - RFID Reader

## Features

- Multi-sensor intrusion detection
- RFID-based secure access
- Real-time monitoring
- Low-cost embedded implementation

## Pin Configuration

| Component           | STM32 Pin | Interface | Direction |
| - | - | - | - |
| LDR                | PA6      | GPIO     | Input    |
| Vibration Sensor   | PA7      | ADC     | Input    |
| RFID (SCL)         | PB6      | I2C      | Output   |
| RFID (SDA)         | PB7      | I2C      | I/O      |

> [!NOTE]
> All module VCC pins are connected to the 5V supply.

