import pyvisa
import sys
import time
import epics
import numpy

MAXF = 16500000000
MINF = 15500000000
MAXP = -14.5
MINP = -20
ai = 0
def initFreq(freq=MINF):
    if (freq>MAXF or freq <MINF):
        print('invalid frequency range')
        return False
    inst.write(':FREQ ' + str(freq))
    if (float(inst.query(':FREQ?').strip())==freq):
        return True
    else:
        print(repr(inst.query(':FREQ?')))
        print('failed to set correct frequency')
        return False

def initPow(pow=MINP):
    if (pow>MAXP or pow <MINP):
        print('invalid power range')
        return False
    inst.write(':POW ' + str(pow) + 'dBm')
    if (int(inst.query(':POW?').strip())==pow):
        return True
    else:
        print(repr(inst.query(':POW?')))
        print('failed to set correct power')
        return False

def updateFreq():
    inst.write(':FREQ UP')
    var = float(inst.query(':FREQ?').strip())
    if (var>MAXF or var <MINF):
        print('invalid frequency range')
        return False
    return True

def updatePow():
    inst.write(':POW UP')
    var = float(inst.query(':POW?').strip())
    if (var>MAXP or var <MINP):
        print('invalid power range')
        return False
    return True

def setFreqInc(inc=200000000):
    inst.write(':FREQ:STEP ' + str(inc))
    if (int(inst.query(':FREQ:STEP?').strip())!=inc):
        print('failed to set freq increment')
        return False
    return True

def setPowInc(inc=0.5):
    inst.write(':POW:STEP ' + str(inc) + ' dBm')
    if (float(inst.query(':POW:STEP?').strip())!=inc):
        print('failed to set increment')
        return False
    return True

def initPnF():
    initFreq()
    initPow()
    setFreqInc()
    setPowInc()

def readFreq():
    var = float(inst.query(':FREQ?').strip())
    return var

def readPow():
    var = float(inst.query(':POW?').strip())
    return var

def powOn():
    a = readFreq()
    b = readPow()
    if a > MAXF or a < MINF or b > MAXP or b < MINP:
        return False
    inst.write(':OUTP ON')
    return True

def powOff():
    inst.write(':OUTP OFF')

def incAtt():
    global ai
    for i in range(12):
        setAtt(2*i)
        time.sleep(1.5)
        ai=epics.caget("SR00RPA01:IN1_DATA_MONITOR")
        time.sleep(1.5)
        ai = numpy.mean(ai)
        readStat()
        f.write(str(2*i)+",   "+str(ai)+"\n")

def readStat():
    global ai
    a = str(readFreq())
    b = str(readPow())
    c = str(epics.caget('car'))
    d = str(ai)
    print('Power = ' + b + '   Frequency = ' + a + '    Attenuation = ' + c + '     Voltage = ' + d)

#set up fast analog input
epics.caput('SR00RPA01:IN1_GAIN_CMD', 'Low')
epics.caput('SR00RPA01:ACQ_TRIGGER_SRC_CMD', 'DISABLED')

#set up connection to signal generator
rm = pyvisa.ResourceManager()
inst = rm.open_resource('TCPIP0::192.168.0.254::inst0::INSTR')

#verifyy connection successful
if (inst.query('*IDN?')!='ANRITSU,MG3692C,211201,3.62\r\n'):
    sys.exit('unexpected string')

#initialise pins as outputs
for i in range(6):
                epics.caput('SR00RPA01:DIGITAL_N'+str(i)+'_DIR_CMD', 'Output')

#initialise Latch Enable to high
epics.caput('SR00RPA01:DIGITAL_N4_STATE_CMD', 'High')


#function to set attenuation
def setAtt(attenVal):
    epics.caput('car', attenVal)

#select switch function
def setSwitch(switchVal):
    if switchVal == 0:
        epics.caput('SR00RPA01:DIGITAL_N5_STATE_CMD','Low' )
    elif switchVal == 1:
        epics.caput('SR00RPA01:DIGITAL_N5_STATE_CMD','High' )

setSwitch(0)
setAtt(0)

initPnF()
epics.caput('SR00RPA01:START_CONT_ACQ_CMD', '1')
#run the script
f = open("autoconfig.txt", "w")

f.write("Power = " + str(readPow())+'\n')
f.write("Frequency = " + str(readFreq())+'\n')

f.write("attenuation, voltage\n")
incAtt()

while(updateFreq()):
        f.write("Frequency = " + str(readFreq())+'\n')
        f.write("attenuation, voltage\n")
        powOn()
        incAtt()
        powOff()
initFreq()
while(updatePow()):
    f.write("Power = " + str(readPow())+'\n')
    while(updateFreq()):
        f.write("Frequency = " + str(readFreq())+'\n')
        f.write("attenuation, voltage\n")
        powOn()
        incAtt()
        powOff()
    initFreq()        

powOff()
inst.close()
epics.caput('SR00RPA01:STOP_ACQ_CMD', '1')
f.close()

f=open("autoconfig.txt","r")

print(f.read())


