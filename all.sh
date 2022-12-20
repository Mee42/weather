set -e
idf.py build
idf.py flash
stty -F /dev/ttyUSB0 115200
tail -f /dev/ttyUSB0
