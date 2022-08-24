import pyvisa
import sys
import time
import epics
import numpy

MAXF = 16500000000 #maximum frequency 
MINF = 15500000000 #minimum frequency 
MAXP = -14.5 #maximum power
MINP = -20 #minimum power
ai = 0

#initialise frequency
def initFreq(freq=MINF):
    if (freq>MAXF or freq <MINF): #check within bounds
        print('invalid frequency range')
        return False
    inst.write(':FREQ ' + str(freq))
    if (float(inst.query(':FREQ?').strip())==freq): #verify it's set correctly 
        return True
    else:
        print(repr(inst.query(':FREQ?')))
        print('failed to set correct frequency')
        return False
#initialise power
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
#update the frequency, making sure it's within bounds
def updateFreq():
    inst.write(':FREQ UP')
    var = float(inst.query(':FREQ?').strip())
    if (var>MAXF or var <MINF):
        print('invalid frequency range')
        return False
    return True
#update the power, making sure it's within bounds
def updatePow():
    inst.write(':POW UP')
    var = float(inst.query(':POW?').strip())
    if (var>MAXP or var <MINP):
        print('invalid power range')
        return False
    return True
#set the frequency increment, making sure it's set correctly
def setFreqInc(inc=200000000):
    inst.write(':FREQ:STEP ' + str(inc))
    if (int(inst.query(':FREQ:STEP?').strip())!=inc):
        print('failed to set freq increment')
        return False
    return True
#set the power increment, making sure it's set correctly
def setPowInc(inc=0.5):
    inst.write(':POW:STEP ' + str(inc) + ' dBm')
    if (float(inst.query(':POW:STEP?').strip())!=inc):
        print('failed to set increment')
        return False
    return True
#call the set up functions
def initPnF():
    initFreq()
    initPow()
    setFreqInc()
    setPowInc()
#function for reading current frequency of signal generator
def readFreq():
    var = float(inst.query(':FREQ?').strip())
    return var
#function for reading current power of signal generator
def readPow():
    var = float(inst.query(':POW?').strip())
    return var
#turn output of signal generator on
def powOn():
    a = readFreq()
    b = readPow()
    if a > MAXF or a < MINF or b > MAXP or b < MINP:
        return False
    inst.write(':OUTP ON')
    return True
#turn output of signal generator off
def powOff():
    inst.write(':OUTP OFF')

#automatically increment the attenuation value in steps of 2
def incAtt():
    global ai
    for i in range(12):
        setAtt(2*i) #set attenuation
        time.sleep(1.5) #wait for epics 
        ai=epics.caget("SR00RPA01:IN1_DATA_MONITOR") #read the analog input values 
        time.sleep(1.5) #wait for epics
        ai = numpy.mean(ai) #take the mean of the array 
        readStat()
        f.write(str(2*i)+",   "+str(ai)+"\n") #write to file

        #read all the values
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

#verify connection successful
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

#initialise the switch and attenuation to a known value        
setSwitch(0)
setAtt(0)

#initialise power and frequency 
initPnF()

#start the fast analog in acquisition
epics.caput('SR00RPA01:START_CONT_ACQ_CMD', '1')
#open a text file in write mode
f = open("autoconfig.txt", "w")

#write the initial power and frequency into text file
f.write("Power = " + str(readPow())+'\n')
f.write("Frequency = " + str(readFreq())+'\n')
#write the headings
f.write("attenuation, voltage\n")
#increment the attenuation level
incAtt()
#continuously loop through all valid frequencies, incrementing by the increment set 
while(updateFreq()):
        f.write("Frequency = " + str(readFreq())+'\n')
        f.write("attenuation, voltage\n")
        powOn()
        incAtt()
        powOff()
initFreq()
#increment through the power and frequencies according to set increments, within the valid range
while(updatePow()):
    f.write("Power = " + str(readPow())+'\n')
    while(updateFreq()):
        f.write("Frequency = " + str(readFreq())+'\n')
        f.write("attenuation, voltage\n")
        powOn()
        incAtt()
        powOff()
    initFreq()        
#turn of output power, close communication with signal generator, stop fast analog acquisition and close the file
powOff()
inst.close()
epics.caput('SR00RPA01:STOP_ACQ_CMD', '1')
f.close()

#print out contents of file
f=open("autoconfig.txt","r")

print(f.read())


