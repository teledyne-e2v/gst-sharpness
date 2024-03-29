# gst-sharpness

# Version 1.0

# About

A Gstreamer plugin to control the PDA of the Optimum 2M module and produce a csv file with all sharpness asociated to PDAs

# Dependencies

The following libraries are required for this plugin.
- v4l-utils
- libv4l-dev
- libgstreamer1.0-dev
- libgstreamer-plugins-base1.0-dev
- meson

#### Debian based system (Jetson): 

```
sudo apt install v4l-utils libv4l-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev meson
```
##### Note : if you are using a Yocto distribution, look at the github to find a .bbappend file which provides all packages to your distribution 


# Compilation

## Ubuntu (Jetson)
First you must make sure that your device's clock is correctly setup.
Otherwise the compilation will fail.

In the **gst-sharpness** folder do:

```
meson build
```
```
ninja -C build
```
```
sudo ninja -C build install
```


## Yocto (IMX)
First you must make sure that your device's clock is correctly setup.
Otherwise the compilation will fail.

In the **gst-sharpness** folder do:

```
meson build
```
```
ninja -C build install
```

# Installation test

To test if the plugin has been correctly install, do:
```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
gst-inspect-1.0 sharpness
```

If the plugin failed to install the following message will be displayed: "No such element or plugin 'sharpness'"

# Uninstall
'''
sudo rm /usr/local/lib/gstreamer-1.0/libgstsharpness.*
'''
# Usage

By default the plugin is installed in /usr/local/lib/gstreamer-1.0. 
It is then required to tell gstreamer where to find it with the command:
```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
```
The plugin can be used in any gstreamer pipeline by adding '''sharpness''', the name of the plugin.

# Pipeline examples:
With fake image pipeline:
```
gst-launch-1.0 videotestsrc ! sharpness ! videoconvert ! ximagesink
```

With simple video stream:
```
gst-launch-1.0 v4l2src ! sharpness ! queue ! videoconvert ! queue ! xvimagesink sync=false
```

# Plugin parameters
 
- work: (usefull only for applications)
    - Description: Set plugin to work                     
    - flags: readable, writable
    - Type: Boolean 
    - Default: true
-  latency: 
    - Description: Latency between command and command effect on gstreamer
    - flags: readable, writable
    - Type: Integer
    - Range: 1 - 50 
    - Default: 3 
- wait-after-start: 
    - Description: Latency between command and command effect on gstreamer
    - flags: readable, writable
    - Type: Integer. 
    - Range: 1 - 50 
    - Default: 30 
- reset: 
    - Description: Reset plugin
    - flags: readable, writable
    - Type: Boolean 
    - Default: false
- roi1x               : Roi coordinates 
    - Description:
    - flags: readable, writable
    - Type: Integer
    - Range: 0 - 1920 
    - Default: 0 
- roi1y: 
    - Description: Roi coordinates
    - flags: readable, writable
    - Type: Integer 
    - Range: 0 - 1080 
    - Default: 0 
- roi2x: 
    - Description: Roi coordinates
    - flags: readable, writable
    - Type: Integer 
    - Range: 0 - 1080 
    - Default: 0                  
- roi2y: 
    - Description: Roi coordinates
    - flags: readable, writable
    - Type: Integer 
    - Range: 0 - 1080 
    - Default: 0                  
- step: 
    - Description: PDA steps
    - flags: readable, writable
    - Type: Integer
    - Range: 1 - 500 
    - Default: 2 
- filename: 
    - Description: name of the csv file, can also be used to change the path
    - flags: readable, writable
    - Type: String
    - Default: "result.csv"
- done: (usefull only for applications)
   - Description: Set to true when the alrgorithm has finish
   - flags: readable, writable
   - Type: Boolean
   - Default: false
