# Microcar
Project for a tiny remote-control car, driven using linear actuators via a magnet, using phone as steering wheel and accelerator.  Really just having fun with some simple electronics, a 3D printer and ChatGPT to do most of the coding.  

<img src=https://github.com/astromikemerri/Microcar/blob/main/animation.gif>

And <A href=https://github.com/astromikemerri/Microcar/blob/main/Testdrive.mov>here is a longer test drive demo</a> to show the full silliness of the project.

In addition to standard accessories like a half-size breadboard and plenty of jumper wires, the hardware that you need, together with some suggested links for buying it, is:
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
      <td>ESP32 board with built-in WiFi</td>
      <td><a href="https://www.amazon.co.uk/dp/B0BYSB66S5">link</a></td>
    </tr>
    <tr>
      <td>2</td>
      <td>A4988 Stepper Driver Modules</td>
      <td>2</td>
      <td>driver boards for bipolar stepper control</td>
      <td><a href="https://www.amazon.co.uk/dp/B083V59HTB">link</a></td>
    </tr>
    <tr>
      <td>3</td>
      <td>Lead-screw Linear Actuators</td>
      <td>2</td>
      <td>Miniature 2-phase steppers with 0.5 mm lead screws and 80mm stroke</td>
      <td><a href="https://www.aliexpress.com/item/1005006040414647.html">link</a></td>
    </tr>
    <tr>
      <td>4</td>
      <td>End-stop Microswitches</td>
      <td>2</td>
      <td>Normally-open switches for homing (LOW when pressed)</td>
      <td><a href="https://www.switchelectronics.co.uk/products/11mm-lever-subminiature-pcb-microswitch-spdt-3a">link</a></td>
    </tr>
    <tr>
      <td>5</td>
      <td>9 V DC Power Supply</td>
      <td>1</td>
      <td>Regulated wall adapter for motor drivers (≥ 1 A recommended)</td>
      <td><a href="https://www.amazon.co.uk/dp/B0DF2LGSHR">link</a></td>
    </tr>
    <tr>
      <td>6</td>
      <td>Multimeter</td>
      <td>1</td>
      <td>Only needed to set the reference voltage on the A4988 boards</td>
      <td><a href="https://www.amazon.co.uk/AstroAI-Digital-Multimeter-Measurement-Electronic/dp/B01ISAMUA6/">link</a></td>
    </tr>
    <tr>
      <td>7</td>
      <td>220 micro Farad Capacitor</td>
      <td>2</td>
      <td>placed across motor power supply input on A4988, to suppress  spikes</td>
      <td><a href="https://www.amazon.co.uk/dp/B07KC99W2K">link</a></td>
    </tr>    
    <tr>
      <td>8</td>
      <td>100k Ohm Resistor</td>
      <td>2</td>
      <td>pulldown on STEP pin of A4988</td>
      <td><a href="https://www.amazon.co.uk/BOJACK-Values-Resistor-Resistors-Assortment/dp/B08FD1XVL6">link</a></td>
    </tr>    
    <tr>
      <td>9</td>
      <td>M2.5x6 screws and washers</td>
      <td>4</td>
      <td>for attaching rails to motor carriages</td>
      <td><a href="https://www.amazon.co.uk/dp/B0CNVP5P4C">link</a></td>
    </tr>    
    <tr>
      <td>10</td>
      <td>1.5mm diameter steel rods</td>
      <td>4</td>
      <td>to use as rails to drive the puck</td>
      <td><a href="https://www.amazon.co.uk/dp/B0D2V6WVT4">link</a></td>
    </tr><tr>
      <td>11</td>
      <td>3mm and 5mm diameter neodynium disc magnets</td>
      <td>1</td>
      <td>to embed in puck and car, to couple them together tgtough the driving surface</td>
      <td><a href="https://www.amazon.co.uk/dp/B0D2V6WVT4">link</a></td>
    </tr>
    <tr>
      <td>12</td>
      <td>Tube of SuperLube</td>
      <td>1</td>
      <td>for greasing the rails to make sure everything moves smoothly</td>
      <td><a href="https://www.amazon.co.uk/dp/B007UU9JMS">link</a></td>
    </tr>
  </tbody>
</table>

The electronics, though it looks a bit messy, is not that complicated, essentially just connecting the  two A4988 stepper motor drivers to the Seeed XIAO ESP32S3 to control the two linear actuators.  The only other electronic hardware is the two microswitches that are attached to the ends of the stepper motor rails, which the software uses to home the two motors, so it knows where the carriages are located on their screw drives.

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

A Fritzing file for the requisite wiring is <A href=https://github.com/astromikemerri/Microcar/blob/main/microcar.fzz>here</a>.

One small piece of electronic tuning you will have to do is to make sure that the A4988 has been set to a current limit appropriate to these mini stepper motors, by adjusting the potentiometer on the A4988 board with a small screwdriver:
1. Power the setup (XIAO on, 9V power supply on).
2. connect voltmeter black to GND, red on the metal top of the pot.
3. Turn the pot slowly until you read a target voltage of approximately 0.24 V.
4. Let it sit 1–2 minutes; if the chip is too hot to touch, turn the voltage down a little.

The other hardware elements are the 3D-printed parts, <A href=https://github.com/astromikemerri/Microcar/blob/main/MotorFittings.3mf>the fittings for the motors</a> and <A href=https://github.com/astromikemerri/Microcar/blob/main/housing.3mf>the housing for the project</a>, not forgetting <A href=https://github.com/astromikemerri/Microcar/blob/main/minicar3mm.3mf>the car itself</a>. I printed these all on a Bambu A1-Mini in PETG, using 0.08mm layers, but there is nothing particularly machine-dependent in the files. 

Superglue one of your 5mm magnets into the puck and a 3mm one into the car, making sure that they are oriented to attract each other.  Slot the microswitch holders over the ends of the linear actuators, and the slide the microswitches themselves over the two pegs.  Screw the rail holders into the carriages of the two actuators. Cut the rails to 104mm, and thread them through the puck and into the rail holders attached to the carriages.  Put the little cube rail ends onto the other end of the rails. Insert the actuators into the tabs in the base.  Once all this is done, the completed hardware should look like this, ready to have the driving surface installed: 

<img src=https://github.com/astromikemerri/Microcar/blob/main/Topless.jpg width=500>

Finally, there is <A href=https://github.com/astromikemerri/Microcar/blob/main/ignition_drive.ino>the code</a>, almost entirely courtesy of ChatGPT.  This sketch sets up the ESP32 as a web server, which serves a webpage to your phone, which uses JavaScript to access the orientation data your phone maintains, which is then sent back to the ESP32 from your phone via websockets, to control the car by driving the linear actuators.  Since iPhones only allow access to this data over a secure connection, you will also need <A href=https://github.com/astromikemerri/Microcar/blob/main/certs.h>this header file</a> with the necessary security certificates in it, which you just have to place in the same folder as the sketch when uploading the code to the ESP32 using the Arduino IDE.

Grease the rails, and now you should be good to go!  Try it a few times without the top on the housing, just to make sure everything is working.  Switch on the motor power supply then plug in the ESP32.  The actuators should home to their microswitches, and then centre in the middle of the play area.  Connect your phone to wifi server "ESP32-Tilt" (password "tilt12345"), then open web page 192.168.4.1, which should give you a link to follow to the secure server page.  When the controller web page loads, press the button to activate the tilt angles, and accept the dialog saying it is OK to activate them. Press the ignition button to start the car moving.  Hold the phone in landscape mode in your hands; tipping left and right steers, and tipping away/toward you accelerates and decelerates.  If you need to stop in a hurry, press the ignition button again. Happy driving! 

And, as a bonus, here is a 3D print file for <A href="https://github.com/astromikemerri/Microcar/blob/main/cones.3mf">those tiny traffic cones</a>.
