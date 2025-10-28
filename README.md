# Microcar
Project for a tiny remote-control car, driven using linear actuators via a magnet, using phone as steering wheel.  Really just having fun with some simple electronics, a 3D printer and ChatGPT to do most of the coding.  

Here is the project with its driving surface removed, to show how it all works -- two miniature linear actuators drive a puck in x and y via the steel rails, and the magnet on the puck drags the tiny car around with it:

<img src=https://github.com/astromikemerri/Microcar/blob/main/Topless.jpg width=500>

And <A href=https://github.com/astromikemerri/Microcar/blob/main/Testdrive.mov>here is a quick test drive demo</a>.

The electronics, though it looks a bit messy, is not that complicated, essentially just connecting two A4988 stepper motor drivers to a Seeed XIAO ESP32S3 to control the two linear actuators.  The only other hardware is two mcroswitches that I attached to the ends of the stepper motor rails, that the code uses to home the two motors, so it knows where the carriages are.

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

<!-- Power & Misc. (informational) -->
<table>
  <thead>
    <tr>
      <th>Signal</th>
      <th>Connection</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>A4988 VDD</td>
      <td>ESP32 3.3 V</td>
      <td>Logic supply</td>
    </tr>
    <tr>
      <td>A4988 GND</td>
      <td>ESP32 GND</td>
      <td>Common ground (logic)</td>
    </tr>
    <tr>
      <td>A4988 VMOT</td>
      <td>External ~9 V</td>
      <td>Motor supply (keep power GND returns stout)</td>
    </tr>
    <tr>
      <td>RESET &amp; SLEEP</td>
      <td>Tied together (A4988)</td>
      <td>Keeps driver awake</td>
    </tr>
    <tr>
      <td>MS1–MS3</td>
      <td>Float / LOW</td>
      <td>Full-step mode (as used in code)</td>
    </tr>
  </tbody>
</table>


<img src=https://github.com/astromikemerri/Microcar/blob/main/circuitwiring.jpg>

A Fritzing file for the requisite wiring is <A href=https://github.com/astromikemerri/Microcar/blob/main/microcar.fzz> here</a>
