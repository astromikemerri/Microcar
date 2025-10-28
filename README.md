# Microcar
Project for a tiny remote-control car, driven using linear actuators via a magnet, using phone as steering wheel.  Really just having fun with some simple electronics, a 3D printer and ChatGPT to do most of the coding.  

Here is the project with its driving surface removed, to show how it all works -- two miniature linear actuators drive a puck in x and y via the steel rails, and the magnet on the puck drags the tiny car around with it:

<img src=https://github.com/astromikemerri/Microcar/blob/main/Topless.jpg width=500>

And <A href=https://github.com/astromikemerri/Microcar/blob/main/Testdrive.mov>here is a quick test drive demo</a>.

The requisit hardware that you need is:
<!-- Hardware Components Table -->
<table>
  <thead>
    <tr>
      <th>#</th>
      <th>Component</th>
      <th>Quantity</th>
      <th>Description</th>
      <th>Link</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>1</td>
      <td>Seeed Studio XIAO ESP32S3</td>
      <td>1</td>
      <td>Main microcontroller board (Wi-Fi + Bluetooth + USB-C)</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>2</td>
      <td>A4988 Stepper Driver Modules</td>
      <td>2</td>
      <td>Pololu-style driver boards for bipolar stepper control</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>3</td>
      <td>Lead-screw Linear Actuators</td>
      <td>2</td>
      <td>Miniature 2-phase steppers with 0.5 mm lead screws</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>4</td>
      <td>End-stop Microswitches</td>
      <td>2</td>
      <td>Normally-open switches for homing (LOW when pressed)</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>5</td>
      <td>9 V DC Power Supply</td>
      <td>1</td>
      <td>Regulated wall adapter for motor drivers (≥ 1 A recommended)</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>6</td>
      <td>Wiring & Connectors</td>
      <td>—</td>
      <td>28 AWG silicone wire, Dupont jumpers, JST connectors, etc.</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>7</td>
      <td>Breadboard or Mounting PCB</td>
      <td>1</td>
      <td>For organizing the A4988 modules and connections</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>8</td>
      <td>3D-Printed Frame Components</td>
      <td>—</td>
      <td>Custom puck, rail blocks, and track assembly (OpenSCAD/STL provided)</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>9</td>
      <td>Screws, Nuts & Spacers</td>
      <td>—</td>
      <td>Hardware for assembling the rail system and securing motors</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
    <tr>
      <td>10</td>
      <td>Optional: Multimeter / USB Power Meter</td>
      <td>—</td>
      <td>For verifying motor supply current and voltage stability</td>
      <td><a href="#">Insert link here</a></td>
    </tr>
  </tbody>
</table>



The electronics, though it looks a bit messy, is not that complicated, essentially just connectingthe  two A4988 stepper motor drivers to the Seeed XIAO ESP32S3 to control the two linear actuators.  The only other hardware is two mcroswitches that are attached to the ends of the stepper motor rails, that the code uses to home the two motors, so it knows where the carriages are.

The pin assignments I used were:
<!-- Stepper Driver Connections -->
<table>
  <thead>
    <tr>
      <th>Axis</th>
      <th>Function</th>
      <th>ESP32 GPIO</th>
      <th>A4988 Pin</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>X</td>
      <td>STEP</td>
      <td>GPIO 1</td>
      <td>STEP</td>
      <td>Step pulse</td>
    </tr>
    <tr>
      <td>X</td>
      <td>DIR</td>
      <td>GPIO 2</td>
      <td>DIR</td>
      <td>LOW = forward (as defined in code)</td>
    </tr>
    <tr>
      <td>X</td>
      <td>ENABLE</td>
      <td>GPIO 3</td>
      <td>EN</td>
      <td>Active LOW (LOW = driver enabled)</td>
    </tr>
    <tr>
      <td>Y</td>
      <td>STEP</td>
      <td>GPIO 9</td>
      <td>STEP</td>
      <td>Step pulse</td>
    </tr>
    <tr>
      <td>Y</td>
      <td>DIR</td>
      <td>GPIO 8</td>
      <td>DIR</td>
      <td>LOW = forward (as defined in code)</td>
    </tr>
    <tr>
      <td>Y</td>
      <td>ENABLE</td>
      <td>GPIO 7</td>
      <td>EN</td>
      <td>Active LOW (LOW = driver enabled)</td>
    </tr>
  </tbody>
</table>

<!-- Endstop Switches -->
<table>
  <thead>
    <tr>
      <th>Axis</th>
      <th>Endstop Input</th>
      <th>ESP32 GPIO</th>
      <th>Electrical</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>X</td>
      <td>Endstop</td>
      <td>GPIO 4</td>
      <td>INPUT_PULLUP</td>
      <td>Normally-open to GND; reads LOW when pressed</td>
    </tr>
    <tr>
      <td>Y</td>
      <td>Endstop</td>
      <td>GPIO 44</td>
      <td>INPUT_PULLUP</td>
      <td>Normally-open to GND; reads LOW when pressed</td>
    </tr>
  </tbody>
</table>

<img src=https://github.com/astromikemerri/Microcar/blob/main/circuitwiring.jpg>

A Fritzing file for the requisite wiring is <A href=https://github.com/astromikemerri/Microcar/blob/main/microcar.fzz> here</a>

The other hardware elements are the 3D-printed parts, <A href=https://github.com/astromikemerri/Microcar/blob/main/MotorFittings.3mf>the fittings for the motors</a> and <A href=https://github.com/astromikemerri/Microcar/blob/main/housing.3mf>the housing for the project</a>. I printed them on a Bambu A1-mini printer in PETG, using 0.08mm layers, but there is nothing particularly machine-dependent in the files.

Finally, there is <A href=https://github.com/astromikemerri/Microcar/blob/main/ignition_drive.ino>the code</a>.  This sketch sets up the ESP32 as a web servre, which serves a webpage to your phone, which uses JavaScript to access the orientation data your pjone maintains, which is then sent back to the ESP32 from your phone via websockets.  Since iPhones only allow access to this data over a secure connection, you will also need <A href=https://github.com/astromikemerri/Microcar/blob/main/certs.h>this header file</a> with the necessary security certificates in it, which you just have to place in the same folder as the sketch when uploading the code to the ESP32 using the Arduino IDE.

(to steer by tipping side to side and accelerate by tipping away from you)
