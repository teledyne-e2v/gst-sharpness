/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2022 Nicolas <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-sharpness
 *
 * FIXME:Describe sharpness here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! sharpness ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <gst/gst.h>
#include <unistd.h>
#include <stdio.h>

#include "gstsharpness.h"
#include "i2c_control.h"

GST_DEBUG_CATEGORY_STATIC(gst_sharpness_debug);
#define GST_CAT_DEFAULT gst_sharpness_debug

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_WORK,
    PROP_LATENCY,
    PROP_WAIT_AFTER_START,
    PROP_RESET,
    PROP_ROI1X,
    PROP_ROI1Y,
    PROP_ROI2X,
    PROP_ROI2Y,
    PROP_STEP,
    PROP_FILENAME,
    PROP_DONE

};

I2CDevice device;
I2CDevice devicepda;
int bus;


// sharpnessConf conf;

int listen = 1;
int frameCount = 0;

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("ANY"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("ANY"));

#define gst_sharpness_parent_class parent_class
G_DEFINE_TYPE(Gstsharpness, gst_sharpness, GST_TYPE_ELEMENT)

static void gst_sharpness_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec);
static void gst_sharpness_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec);

static GstFlowReturn gst_sharpness_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);

static void gst_sharpness_finalize(GObject *object);

#define TYPE_sharpness_STATUS (sharpness_status_get_type())
static GType
sharpness_status_get_type(void)
{
    static GType sharpness_status = 0;

    if (!sharpness_status)
    {
        static const GEnumValue status[] =
            {
                {PENDING, "Pending", "pending"},
                {IN_PROGRESS, "In progress", "in_progress"},
                {COMPLETED, "Completed", "completed"},
                {0, NULL, NULL}};

        sharpness_status = g_enum_register_static("sharpnessStatus", status);
    }

    return sharpness_status;
}

#define TYPE_DEBUG_LEVEL (debug_level_get_type())
static GType
debug_level_get_type(void)
{
    static GType debug_level = 0;

    if (!debug_level)
    {
        static const GEnumValue status[] =
            {
                {NONE, "None", "none"},
                {MINIMAL, "Minimal", "minimal"},
                {FULL, "Full", "full"},
                {0, NULL, NULL}};

        debug_level = g_enum_register_static("DebugLevel", status);
    }

    return debug_level;
}

/**
 * @brief Prevent the ROI from protuding from the image
 */
static void checkRoi()
{
    // Prevent the ROI from being to close to the very end of the frame as it migth crash when calculating the sharpness
    if (roi.x > 1916)
    {
        roi.x = 1916;
    }

    if (roi.y > 1076)
    {
        roi.y = 1076;
    }

    // Prevent the ROI from going outsides the bounds of the image
    if (roi.x + roi.width >= 1920)
    {
        roi.width -= ((roi.x + roi.width) - 1920);
    }

    if (roi.y + roi.height > 1080)
    {
        roi.height -= ((roi.y + roi.height) - 1080);
    }
}

/* GObject vmethod implementations */

/* initialize the sharpness's class */
static void gst_sharpness_class_init(GstsharpnessClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;

    gobject_class->set_property = gst_sharpness_set_property;
    gobject_class->get_property = gst_sharpness_get_property;
    gobject_class->finalize = gst_sharpness_finalize;

    g_object_class_install_property(gobject_class, PROP_LATENCY,
                                    g_param_spec_int("latency", "Latency", "Latency between command and command effect on gstreamer",
                                                     1, 50, 3, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_WAIT_AFTER_START,
                                    g_param_spec_int("wait_after_start", "Wait_after_start", "Latency between command and command effect on gstreamer",
                                                     1, 50, 30, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_WORK,
                                    g_param_spec_boolean("work", "Work",
                                                         "Set plugin to work",
                                                         TRUE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_RESET,
                                    g_param_spec_boolean("reset", "Reset",
                                                         "Reset plugin",
                                                         TRUE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_DONE,
                                    g_param_spec_boolean("done", "Done",
                                                         "Set to true when the alrgorithm has finish",
                                                         TRUE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_STEP,
                                    g_param_spec_int("step", "Step", "PDA steps",
                                                     1, 500, 2, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_FILENAME,
                                    g_param_spec_string("filename", "Filename",
                                                        "name of the csv file, can also be used to change the path",
                                                        "result.csv", G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "sharpness",
                                         "FIXME:Generic",
                                         "sharpness of snappy2M module",
                                         "Esisar-PI2022 <<user@hostname.org>>");
    g_object_class_install_property(gobject_class, PROP_ROI1X,
                                    g_param_spec_int("roi1x", "Roi1x", "Roi coordinates", 0, 1920, 0, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_ROI1Y,
                                    g_param_spec_int("roi1y", "Roi1y", "Roi coordinates", 0, 1080, 0, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_ROI2X,
                                    g_param_spec_int("roi2x", "Roi2x", "Roi coordinates", 0, 1920, 1920, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_ROI2Y,
                                    g_param_spec_int("roi2y", "Roi2y", "Roi coordinates", 0, 1080, 1080, G_PARAM_READWRITE));

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_sharpness_init(Gstsharpness *sharpness)
{
    sharpness->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_chain_function(sharpness->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_sharpness_chain));
    GST_PAD_SET_PROXY_CAPS(sharpness->sinkpad);
    gst_element_add_pad(GST_ELEMENT(sharpness), sharpness->sinkpad);

    sharpness->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(sharpness->srcpad);
    gst_element_add_pad(GST_ELEMENT(sharpness), sharpness->srcpad);

    sharpness->work = TRUE;
    sharpness->latency = 3;
    sharpness->wait_after_start = 30;
    sharpness->ROI1x = 0;
    sharpness->ROI1y = 0;
    sharpness->ROI2x = 1920;
    sharpness->ROI2y = 1080;
    sharpness->step = 2;
    sharpness->reset = false;
    sharpness->done=false;
    sharpness->filename = (char *)malloc(sizeof(char) * 200);
    strncpy(sharpness->filename, "result.csv", 12);

    i2cInit(&device, &devicepda, &bus);
}

static void gst_sharpness_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec)
{
    Gstsharpness *sharpness = GST_sharpness(object);

    switch (prop_id)
    {
    case PROP_WORK:
        sharpness->work = g_value_get_boolean(value);
        break;
    case PROP_LATENCY:
        sharpness->latency = g_value_get_int(value);
        break;
    case PROP_WAIT_AFTER_START:
        sharpness->wait_after_start = g_value_get_int(value);
        break;

    case PROP_RESET:
        sharpness->reset = g_value_get_boolean(value);
        break;
    case PROP_DONE:
        sharpness->done = g_value_get_boolean(value);
        break;
    case PROP_ROI1X:
        sharpness->ROI1x = g_value_get_int(value);
        break;
    case PROP_ROI1Y:
        sharpness->ROI1y = g_value_get_int(value);
        break;
    case PROP_ROI2X:
        sharpness->ROI2x = g_value_get_int(value);
        break;
    case PROP_ROI2Y:
        sharpness->ROI2y = g_value_get_int(value);
        break;
    case PROP_FILENAME:
        strncpy(sharpness->filename, g_value_get_string(value), 200);
        break;
    case PROP_STEP:
        sharpness->step = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_sharpness_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec)
{
    Gstsharpness *sharpness = GST_sharpness(object);

    switch (prop_id)
    {
    case PROP_WORK:
        g_value_set_boolean(value, sharpness->work);
        break;
    case PROP_LATENCY:
        g_value_set_int(value, sharpness->latency);
        break;
    case PROP_WAIT_AFTER_START:
        g_value_set_int(value, sharpness->wait_after_start);
        break;
    case PROP_RESET:
        g_value_set_boolean(value, sharpness->reset);
        break;
    case PROP_DONE:
        g_value_set_boolean(value, sharpness->done);
        break;
    case PROP_ROI1X:
        g_value_set_int(value, sharpness->ROI1x);
        break;
    case PROP_ROI1Y:
        g_value_set_int(value, sharpness->ROI1y);
        break;
    case PROP_ROI2X:
        g_value_set_int(value, sharpness->ROI2x);
        break;
    case PROP_ROI2Y:
        g_value_set_int(value, sharpness->ROI2y);
        break;
    case PROP_STEP:
        g_value_set_int(value, sharpness->step);
        break;
    case PROP_FILENAME:
        g_value_set_string(value, sharpness->filename);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_sharpness_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{

    Gstsharpness *sharpness = GST_sharpness(parent);

    if(sharpness->reset)
    {
        actual_pda=-90;
        done=false;
        sharpness->reset=false;
	sharpness->done=false;
		tmp_frame = 0;
    }

    if (sharpness->work && sharpness->wait_after_start < frame)
    {

        roi.x = sharpness->ROI1x;
        roi.y = sharpness->ROI1y;
        roi.height = sharpness->ROI2y - sharpness->ROI1y;
        roi.width = sharpness->ROI2x - sharpness->ROI1x;

        // printf("pda : %d sharp : %ld\n",actual_pda, sharp[frame-sharpness->wait_after_start]);
        

	if (actual_pda < 800 || tmp_frame < sharpness->latency)
        {

		if(actual_pda >= 800)
		{
					sharp[(actual_pda + 90) / sharpness->step + tmp_frame ] = getSharpness(pad, buf, roi);
			tmp_frame++;
		}
		else
		{
			sharp[(actual_pda + 90) / sharpness->step] = getSharpness(pad, buf, roi);
			actual_pda += sharpness->step;
            		write_VdacPda(devicepda, bus, actual_pda);	
		}
	}
        else if (!done)
        {
            done = true;
            last_frame = frame;
        }
        // printf("tmp : %d, frame %d\n",sharpness->latency + last_frame, frame);
        if (done && last_frame + sharpness->latency == frame) // WRITE CSV
        {
            FILE *file;
            file = fopen(sharpness->filename, "w+");
            fprintf(file, "pda, sharpness\n");
            for (int i = 0; i < (int)((800 + 90) / sharpness->step); i += 1)
            {
                fprintf(file, "%d, %ld\n", i*sharpness->step-90, sharp[i + sharpness->latency]);
            }
            fclose(file);
		sharpness->done=true;
            printf("FINISHED\n");
		tmp_frame = 0;
        }
    }
    frame++;

    /* just push out the incoming buffer */
    return gst_pad_push(sharpness->srcpad, buf);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean sharpness_init(GstPlugin *sharpness)
{
    /* debug category for fltering log messages
     *
     * exchange the string 'Template sharpness' with your description
     */
    GST_DEBUG_CATEGORY_INIT(gst_sharpness_debug, "sharpness",
                            0, "Template sharpness");

    return gst_element_register(sharpness, "sharpness", GST_RANK_NONE,
                                GST_TYPE_sharpness);
}

static void gst_sharpness_finalize(GObject *object)
{
    disable_VdacPda(devicepda, bus);
    i2c_close(bus);
    g_print("Bus closed\n");
    freeDebugInfo();
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstsharpness"
#endif

/* gstreamer looks for this structure to register sharpnesss
 *
 * exchange the string 'Template sharpness' with your sharpness description
 */
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    sharpness,
    "Template sharpness",
    sharpness_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/")
