# Introduction

This Red Pitaya driver support is a truncated version of the [Australian Synchrotron driver support](https://github.com/AustralianSynchrotron/redpitaya-epics), containing only the process variables for the digital pins. In order to obtain the full selection of process variables (analog pins, wave form generator, oscilloscope), install the Australia Synchrotron version instead. 

# Installation

The Red Pitaya uses an ubuntu operating system and provides a command line interface which can be remotely connected to via ssh on linux, or putty on windows. Detailed instructions for establishing the remote connection can be found in the official [redpitaya documentation](https://redpitaya.readthedocs.io/en/latest/developerGuide/software/console/ssh/ssh.html). 

Firstly, ensure that epics base and the asyn package are installed by following the official [installation documentation](https://docs.epics-controls.org/projects/how-tos/en/latest/getting-started/installation.html#:~:text=%20Installation%20on%20Linux%2FUNIX%2FDARWIN%20%28Mac%29%20%C2%B6%20%201,%28NAME%29%20works%20if%20it%20is%20defined...%20More%20). Due to the slow speed of compilation on the RedPitaya, it is advised that the packages are cross-compiled on a more powerful pc. Instructions are presented in the README.md file in the [Australian Synchrotron Github](https://github.com/AustralianSynchrotron/redpitaya-epics).
With EPICS and asyn installed, the next step is to clone this github repository to a desired directory on your RedPitaya. For example, the directory used with the red ptiaya connected to the IP address 148.79.100.64, RP1, is the /root directory. The following command can be used to clone the github repository: 

`git clone https://github.com/Mrwnlflt/rp_EPICS.git .`

The next step is to edit the RELEASE file located inside the configure directory. The release file contains a user specified list of other directories containing the files needed by the current directory. The release file can be found in $(TOP)/configure/RELEASE where $(TOP) is installation directory. In the case of RP1, the top directory is /root/redpitaya-epics. The variables ASYN and EPICS_BASE inside the release file should be edited to include the install locations of asyn and epics-base for your device. For RP1, the RELEASE file looks as follows 

```
TEMPLATE_TOP=${EPICS_BASE}/templates/makeBaseApp/top

ASYN=/root/EPICS/support/asyn

# EPICS_BASE usually appears last so other apps can override stuff:
EPICS_BASE=/root/EPICS/epics-base
```
Having completed this, change to the top directory of the driver support, e.g. `cd /root/redpitaya-epics` and run `make`. 

# Launching the IOC 

Before launching the IOC, it's necessary to disable the nginx webserver from launching on startup using the commands 

```
systemctl stop redpitaya_nginx
systemctl disable redpitaya_nginx
```

Following this, the FPGA must be loaded before the IOC can be run. Assuming the package was built successfully, two files should be present in the `$(TOP)/bin/linux-arm` directory. Execute the load_fpga_image.sh file as root to load the fpga image. It may be necessary to change the permissions of the file to make it executable. 

```
chmod u+x load_fpga_image.sh 
sudo ./load_fpga_image.sh
```
You can now run the IOC by changing to the $(TOP)/iocBoot/iocRedPitayaTest directory and executing the st.cmd script. Again, it may be necessary to change permissions to executable.

```
chmod u+x st.cmd 
sudo ./st.cmd
```
# Using the IOC

The previous command should have launched the IOC and displayed the IOC shell which can be interacted with. Begin by typing the command `help` for a list of available commands. The `dbl` command provides a full list of process variables avaiable with the IOC. The process variables in this redpitaya-epics IOC follow the template 
```
RP1:DIGITAL_<N,P><0...7>_DIR_CMD
RP1:DIGITAL_<N,P><0...7>_STATE_CMD
RP1:DIGITAL_<N,P><0...7>_DIR_STATUS
RP1:DIGITAL_<N,P><0...7>_STATE_STATUS
```
Where N and P represent the column and 0...7 represents pins 0 through 7 for each column. The DIR_CMD process variables allow you to set the direction of the pin as input or output, while the STATE_CMD process variables allow you to set the digital pins to Low or High corresponding to 0 or 3.3V. 

There are many options for interacting with the process variables, e.g. MEDM, CSS, caQtDm, PyEPICS, but the simplest is EPICS command line tools. In order to the set pin N3 as an output pin with a value of High, you can launch a new terminal (ctrl+alt+t) and use the following commands:

```
caput RP1:DIGITAL_N3_DIR_CMD Output
caput RP1:DIGITAL_N3_STATE_CMD High
```
Alternatively, you can set pin P7 as an output with value of Low using:

```
caput RP1:DIGITAL_P7_DIR_CMD Output
caput RP1:DIGITAL_P7_STATE_CMD Low
```
You can also retrieve the values of the pin direction and status using commands like:
```
caget RP1:DIGITAL_P7_DIR_STATUS
caget RP1:DIGITAL_P7_STATE_STATUS
```
And continuously monitor a process variable with the command: 

`camonitor RP1:DIGITAL_P2_DIR_STATUS`


# Modifying the IOC

The heart of the IOC is the database. The database contains all the records which correspond to the process variables. The database is produced by a template file which can be found in the directory $(TOP)/RedPitayaSup/Db/redpitaya_digital_pin.template and a substitution file found in the directory $(TOP)/RedPitayaTestApp/Db/Substitutions. The template file contains records such as 

```
record (bo, "$(DEVICE):DIGITAL_N$(PIN)_DIR_CMD") {
   field (DESC, "Pin N$(PIN) direction command")
   field (SCAN, ".1 second")
   field (DTYP, "asynInt32")
   field (OUT,  "@asyn($(PORT), $(PIN), 0) NDPDIR")
   field (ZNAM, "Input")
   field (ONAM, "Output")
   field (VAL, "1")
}
```
These records are fully configurable. You can modify the type (bo stands for binary output in the example above), the name ("$(DEVICE):DIGITAL_N$(PIN)_DIR_CMD" can be replaced with the desired name) and the description (edit the string attached to the DESC field). You can add and remove fields, or edit the values of the ones present. A full list of record fields, what they do and how to use them, can be found in the [Record Reference Manual](https://epics.anl.gov/base/R7-0/6-docs/RecordReference.html). For example, the SCAN field specifies the scanning period for periodic record scans or the scan type for non-periodic record scans; you can modify it to `field(SCAN, "passive")` for the record processing to be triggered by other records or channel access. `field (SCAN, ".1 second")` sets a periodic scan rate where the record processes every 0.1 seconds. A more comprehensive description of EPICS records is given in [EPICS Process Databse Concepts](https://docs.epics-controls.org/en/latest/guides/EPICS_Process_Database_Concepts.html#epics-process-database-concepts).

Another important field to edit is `VAL`, which sets the initial value that the IOC boots up with. 
$(DEVICE) - base pv name, $(PIN) - pin number, and $(PORT) - asyn portname, represent macros that allow the substitution of the variable with values defined in the substitutions files. That is: 

```
file "db/redpitaya_digital_pin.template"
{
   pattern  {DEVICE,    PORT, PIN}
            {RP1, RP,   0  }
            {RP1, RP,   1  }
            {RP1, RP,   2  }
            {RP1, RP,   3  }
            {RP1, RP,   4  }
            {RP1, RP,   5  }
            {RP1, RP,   6  }
            {RP1, RP,   7  }
}
```
Substitutes the variables inside the template file, expanding to produce process variables for each row as defined by the pattern above. RP1 can be changed to edit the base pv name. 

# Automatic booting of IOC

# Test Program 
