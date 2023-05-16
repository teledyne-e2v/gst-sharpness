# gst-sharpness

# Version 1.0.0

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

- freeze:
    - Type: boolean
    - Default value: false
    - Description: Freeze the stream without blocking the pipeline

- listen:
    - Type: boolean
    - Default value: true
    - Description: Listen for user inputs in the terminal
