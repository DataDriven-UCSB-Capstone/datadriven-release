# Device Tests
Tests to check device functionality

# General Device Information
- MCP2515 uses 8MHz oscillator
- [Zephyr bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings.html)

# Pinout
|Pin|Function|Alias|
|---|---|---|
|P0.00|LED1||
|P0.01|LED2||
|P0.02|LED3||
|P0.03|LED4||
|P0.04|LED5||
|P0.05 to P0.07|NC||
|P0.08|OPT1|P4.3|
|P0.09|OPT2|P4.4|
|P0.10|OPT3|P4.4|
|P0.11|OPT4|P4.5|
|P0.12|OPT5|P4.6|
|P0.13|CAN SCK|SPI CLK|
|P0.14|CAN CS|SPI CS|
|P0.15|CAN MOSI|SPI MOSI|
|P0.16|CAN MISO|SPI MISO|
|P0.17|CAN INT|SPI INT|
|P0.18 to P0.21|NC||
|P0.22|ACC INT|I2C INT|
|P0.23|ACC SCL|I2C SCL|
|P0.24|ACC SDA|I2C SDA|
|P0.25|NC||
|P0.26|USB CTS|UART CTS|
|P0.27|USB RTS|UART RTS|
|P0.28|USB TX|UART TX|
|P0.29|USB RX|UART RX|
|P0.30|BTN1||
|P0.31|BTN2||

# Extra header information
- P1 is connected to SIM
- P2 is connected to the power supply
- P3 is Debug connector
- P4 is for OPT devices
- P5 is for 5V
- P6 is for 3V3

# Additional breakouts
|Pin|Function|
|---|---|
|P1.1|SIM 1V8|
|P1.2|SIM GND|
|P1.3|SIM IO|
|P1.4|NC|
|P1.5|SIM RST|
|P1.6|SIM CLK|
|P1.7 to P1.10|NC|
|||
|P2.1|GND|
|P2.2|VEXT_TB|
|||
|P3.1|VDD|
|P3.2|SWDIO|
|P3.3|GND|
|P3.4|SWDCLK|
|P3.5|GND|
|P3.6 to P3.8|NC|
|P3.9|GND|
|P3.10|RESET|
|||
|P4.1|NC|
|P4.2|GND|
|P4.3|OPT1|
|P4.4|OPT2|
|P4.5|OPT3|
|P4.6|OPT4|
|P4.7|OPT5|
|P4.8|VDD_GPIO_3V|
|||
|P5.1|VDD|
|P5.2|GND|
|||
|P6.1|VSUPPLY|
|P6.2|GND|

