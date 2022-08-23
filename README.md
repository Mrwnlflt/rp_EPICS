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

# Modifying the IOC

# Automatic booting of IOC

# Test Program 
