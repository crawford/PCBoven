PCBoven
=======

Reflow your toast.

'''Note''': Use this design at your own risk. I am not responsible for damaged
hardware or scorched apartments.

This is a USB host controlled DIY reflow oven. The aim of the project is to take
a cheap toaster oven and turn it into a temperature controlled reflow oven. The
oven itself is retrofitted with a custom control circuit which communicates over
USB with a host. The onboard oven controller is relatively simple, taking simple
commands (e.g. set target temperature) and reporting its state periodically. The
host application is the component that actually interpolates the reflow profile
and intructs the oven to hit target temperatures at certain times.

This implementation is broken into three components: firmware, device driver,
and control application, detailed below.

#Firmware#
The oven is controlled via the ATmega32u4 (in my case, the Adafruit ATmega32u4
breakout board), though another USB-enabled Atmel MCU could be specified in the
build process. The controller is designed to control two heating elements
independantly of one another in order to achieve a more even heating. Please,
refer to the [[schematic]] for details.

The firmware on the controller is communicates with the device driver using a
bulk endpoint (commands sent from the host to the device) and an interrupt
endpoint (periodic state (sent from the device to the host). Both endpoints
sent data using the same format (one for commands and one for state updates),
a packed structure of flags, for simplicity.

The firmware is responsible for properly controlling the temperature. The goal
is to quickly and accurately regulate the temperature with a fast response time
and low overshoot. Currently, there is no control algorithm and the performance
is less than ideal. Eventually, I'll implement a PD or PID controller (but no
autotuning (sorry T-Pain) so you'll have to provide your own values).

#Device Driver#
The device driver is written as a loadable kernel module for Linux (tested on
version 3.2 of the kernel). It registers itself as a miscellaneous device on
module load which creates the /dev/pcboven node (this node can only be used to
call ioctls). The driver also registers a USB device driver which gets loaded
when the oven controlled is connected to the host. The USB device driver
exposes a series of sysfs entries (e.g. probe temperature, fault flags, target
oven temperature) which can be used for debugging.

Control and monitoring of the oven is achieved through ioctl calls to the
/dev/pcboven node. This node is capable of sending SIGIO signals to the current
file owner. The signals are sent whenever a new state is received from the oven
or when the oven has been connected/disconnected. This mechanism is used
heavily by the control application and allows it to asynchronously monitor
connectivity and state.

#Control Application#
The control application is a relatively simple GUI front-end to this system.
It takes one command-line parameter, the path to the reflow profile. The profile
consists of a series of timestamps and temperatures which describe the intended
temperature/time curve (an example profile is included in the application
directory). The application interpolates the profile into a series of
temperature targets at one second intervals. This is what allows the oven to
smoothly follow the reflow curve.

The control application allows the user to monitor and control the reflow
sequence. At any point, the user can stop or start the sequence and view system
statistics. A temperature/time graph for both the target and actual oven
temperature is created in realtime. This allows the user to calibrate the oven
before use and to ensure that the sequence reached adequate temperatures during
use.
