## ShaderPort

ShaderPort is a tool for quickly creating visualizations and GUIs to debug microcontrollers that do not have a monitor and keyboard/mouse connected.

It can also be used to quickly add cross-platform GUI capabilities to any program, without the hassle of importing or compiling against window libraries.

## Usage

1. Download the ShaderPort header appropriate for the device you want to debug.
    * [shaderport_tcp.h](shaderport_tcp.h): If your device supports TCP/IP (e.g. an RPi running linux or just an ordinary computer).
    * [shaderport_serial.h](shaderport_serial.h): If your device supports serial (e.g. an Arduino connected via UART to USB or a bluetooth dongle).
2. Write code that visualizes the stuff you want to see, or create some GUI sliders and buttons.
3. Download the ShaderPort binary on the computer you want to debug from, and connect to the device you want to debug.

## Hotkeys

* `ctrl+s`: Take a screenshot or record a video
* `ctrl+w`: Set window size
* `escape`: Close the application
