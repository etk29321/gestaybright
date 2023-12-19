# DMX control for GE Color Cycle Staybright christmas lights

Lowes has some [GE branded LED multifunction string lights](https://www.lowes.com/pd/GE-Color-Choice-600-Count-124-4-ft-Multi-Function-Warm-White-Multicolor-LED-Christmas-String-Lights/5002160651) that have proven to be quite reliable for me.  They are 28V DC powered and can string atleast 1200 lights together easily on one string.  Reversing the polarity switches them from warm white to multicolor.  The included controller has many standard effects as well.  This project adds DMX and E131 capabilities to the lights utilizing an ESP32.  Configuration of the controler is done via a very simple web interface to allow selection between the standard built-in effects, hardwired DMX, or E131 over wifi.  This allows control of these lights (on/off, color, and intensity) via [xLights](xlights.org).

## Hardware

- Controller from the [light string](https://www.lowes.com/pd/GE-Color-Choice-600-Count-124-4-ft-Multi-Function-Warm-White-Multicolor-LED-Christmas-String-Lights/5002160651)
- ESP32 Dev board [Amazon link](https://www.amazon.com/gp/product/B08D5ZD528/ref=ppx_yo_dt_b_asin_title_o04_s00?ie=UTF8&psc=1)
- DC-DC Buck Converter [Amazon link](https://www.amazon.com/dp/B07JWGN1F6?psc=1&ref=ppx_yo2ov_dt_b_product_details)
- RS485 board (optional if you dont want hardwired DMX) [Amazon link](https://www.amazon.com/gp/product/B00NIOLNAG/ref=ppx_yo_dt_b_asin_title_o03_s02?ie=UTF8&psc=1)
- Female 5-pin DMX jack (optional) [Amazon link](https://www.amazon.com/gp/product/B0B1X5CT5W/ref=ppx_yo_dt_b_asin_title_o03_s04?ie=UTF8&psc=1)

### Controller Modifications

The supplied controller gives us most of the hardware we need to drive the lights.  We do have to remove the onboard chip and wire in the ESP32 to run our own code though.  The chip to remove is circled in the picture below.  You can try desoldering it but I had good luck just cutting the pins with a utility knife.  Once the chip is removed we need to attach wires to the board that will go to the GPIO pins on the ESP32.  Tracing the orginal control chip outputs, the easiest place to tap in is next to the two resisters pointed to in the picture.  A 3.3V PWM single applied at these points will activate the transistors and produce a propotional output voltage of 0-28V on one of the output pins while pulling the opposite pin to ground.

Additionally, I soldered the buck converter to the 28V input terminals of the board.  This was a convient place to connect.

![ge_controller_wiring](https://github.com/etk29321/gestaybright/assets/13752726/6286fbbd-0df3-49bf-b58c-00c41cf57e0f)

### Buck converter

While it may be possible to power the ESP32 from the stock controller we will need a stable 5V for the RS485 TTL logic.  The buck converter fills the bill here.  The one I used is adjustable so after wiring you will need to attach a multimeter and adjust its output to 5V before wiring the RS485 or ESP32.  Attached one output to the VIN and GND pins of the ESP32 and another to the VCC and GND pins of the RS485 board.

### ESP32 Wiring

| ESP32 Pin | Wired to                     |
|-----------|------------------------------|
|  GPIO12   | GE Controller White Pin      |
|  GPIO14   | GE Controller Multicolor Pin |
|  GPIO16   | RS485 DI pin                 |
|  GPIO17   | RS485 RO pin                 |
|  GPIO21   | RS485 RE and DE pins         |



