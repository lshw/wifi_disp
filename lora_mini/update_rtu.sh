killall minicom 2>/dev/null
killall avrdude 2>/dev/null
avrdude  -v  -c avrdude.conf -patmega168 -carduino -P/dev/ttyUSB0 -b19200 -D -Uflash:w:/tmp/build/lora_mini.ino.hex:i
