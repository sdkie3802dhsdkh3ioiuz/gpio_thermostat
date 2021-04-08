# gpio_thermostat
OpenBSD daemons to control and read temperature from connected Arduino
A Raspberry Pi called Talker is connected to an Arduino. The Arduino
reads the temperature from a DHT22 sensor and converts it to binary and
sends it out over the connected pins to Talker.

Talker reads its GPIO pins and converts the number from binary to decimal.
If the temperature exceeds the specified maxiumum, we turn the thermostat
pin on. (It's connected to a relay to feed power to a wall-unit air conditioner.)
The daemon on Talker is from gpiotalker.c. It sends out the temperature as a decimal
to Listener. 

Listener is the other Raspberry Pi running OpenBSD. I have them connected via
VPN. Listener is in a remote location. It gets the decimal temperature from Talker
and converts it to binary. Then it turns on the appropriate pins on a connected
breadboard to light up some LEDs, visually representing the temperature in binary.
