import serial
import re

# regexes
comma_reg = re.compile(",")

# power meter serial format
PORT = '/dev/ttyUSB0'
BAUD = 115200
TIMEOUT = 10 # seconds
BYTESIZE = 8
PARITY = serial.PARITY_NONE

# time in seconds to monitor
def monitor(time):
    ser = serial.Serial(PORT, 115200, timeout=TIMEOUT, bytesize=BYTESIZE, parity=PARITY)
    
    # flush backed up data
    ser.flushInput()
   
    # discard first line (tends to be off)
    ser.readline()
    
    results = []
    for i in range(0, time):
        results.append(ser.readline())
     
    ser.close()

    mapresults = []
    for r in results:
        toks = re.split(comma_reg, r)
        d = {'power':toks[3],  # 1/10 W
             'voltage':toks[4],  # 1/10 V
             'current':toks[5]   # 1/1000 A
             }
        mapresults.append(d) 

    return mapresults
