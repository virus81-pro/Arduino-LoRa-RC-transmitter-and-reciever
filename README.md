# Arduino Lora based Radio Controller

An Arduino powered long range radio controller for use with rc planes, cars, boats, etc.

<p align="left">
<img src="img1.jpg" width="622" height="759"/>
</p>

## Features:
- 8 proportional channels with Reverse, Endpoints, Subtrim, Cut, Failsafe
- 2 digital toggle channels
- 5 model memory
- Dualrate & expo for Ail, Ele, Rud
- Throttle curve with 5 points
- Safety (cut) switch
- 10 free mixer slots per model
- 2 timers; Throttle timer and free stopwatch
- Sticks calibration
- Audio beep feedback

## License:
[MIT license](https://choosealicense.com/licenses/mit/)

## Hardware:
#### Transmitter
- 2x Atmega328p microcontrollers
- 1x Semtech SX1276/77/78/79 based rf modules 
- 128x64 KS0108 based LCD, or any 128x64 lcd (provide own driver code). See the schematics
- 3x push buttons
- Additional support components. See the schematics

#### Receiver
- 1x Atmega328p mcu
- 1x Semtech SX1276/77/78/79 based rf module
- Additional support components

<p align="left">
<img src="layout.jpg" width="691" height="649"/>
</p>

## Firmware:
The code compiles on Arduino IDE 1.8.9 and higher with board set to Arduino Uno. 
The transmitter code is in mtx (master mcu) and stx (slave mcu) folders. The receiver mcu code is in the rx folder.

### User Interface
Below is a screenshot of the user interface. 
Three buttons are used for navigation; Up, Select, Down. Long pressing Select in any screen acts as Back. 
Long pressing up or down in mixer screen pops up a mixer context menu there. On homescreen, holding the Down key
reveals the extra 2 digital channels (9 and 10).

<p align="left">
<img src="img2.png" width="1074" height="838"/>
</p>

### Example mixes:

#### Note:
1. Default mapping is Ail->Ch1, Ele->Ch2, Thrt->Ch3, Rud->Ch4, unless overridden in mixer.
2. In case after mix, the servos are moving in wrong direction, it can be solved by negating 
  the applied weights, or reversing the servo direction in Outputs screen. 

Unless otherwise specified, offset is 0, differential is 0

#### Vtail
Left servo in ch2, right servo in ch4
1. Ch2 =  50%Rud + -50%Ele
2. Ch4 = -50%Rud + -50%Ele

#### Elevon
Left servo in Ch1, right servo in Ch2
1. Ch1 = -50%Ail + -50%Ele
2. Ch2 =  50%Ail + -50%Ele

#### Aileron differential
Left aileron in Ch1, right aileron in Ch8
1. Ch1 = -100%Ail{-25%Diff}
2. Ch8 =  100%Ail{ 25%Diff}

#### Crow mix
We would like to setup crow braking on our plane. 
Left ail servo in Ch1, Right Ail servo in Ch8,
left flap servo in Ch5, right flap servo in Ch6.
Using switch 3 position SwC to control the mix. 
When SwC in in upper or middle position, normal aileron action occurs. Half flaps are
deployed when SwC is in middle position.
When SwC is in lower position, both ailerons move upward and full flaps are deployed
thus providing crow braking feature.
1. Ch1 = -100%Ail{-25%Diff} + 50SwC{100%Diff}
2. Ch8 =  100%Ail{ 25%Diff} + 50SwC{100%Diff}
3. Ch5 = -50SwC{-50offset}
4. Ch5 = -50SwC{-50offset}

#### Differential thrust (twin motor)
Left motor in Ch3, right motor in Ch7. 
We can use a switch e.g SwD to turn the mix on or off. 
1. Virt1 = 40%Rud * 100%SwD{100%Diff}
1. Ch3  = 100%Thrt + 100%Virt1
2. Ch7  = 100%Thrt + -100%Virt1

#### Elevator throttle mixing
When the throttle is increased, some down elevator can be added. 
1. Ch2 = -100%Ele + -20%thr{-20 offset}

#### Variable Steering Depending on Throttle Position
This reduces the sensitivity of nosewheel steering as the throttle setting increases.
Suppose we want a steering rate of 100% when the throttle is closed, 
reducing to zero rate (no nosewheel steering) at full throttle. 
What is required is a Multiply mix added to the nosewheel servo channel.
1. Ch6 = 100%yaw * -50%thr{50 offset}

#### Trim with the knob
Example: We would like to trim our planes elevator with the knob while in flight
1. Ch2 = -100%Ele + -30%knob
Note: This doesnt change the subtrim so we have to manually remove this mix later
and adjust the subtrim manually when not in flight 

## Limitations:
1. No trim buttons. A workaround is to trim with the knob. 
2. No basic telemetry support. 
3. Presently the RF transmission occurs at a fixed frequency (433Mhz) thus may interfere
with other devices. 

## To do:
1. Implement frequency hopping
