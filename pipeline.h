#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include <glib.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/app.h>

// Gstreamer pipeline
typedef struct _Pipeline{

GstState pipeState;
char stream_uri[128];
bool loop;

GstElement *pipeline;
GstElement *videosrc;
GstElement *convert;
GstElement *scale;
GstElement *appsink;

GstBus *bus;
GstMessage *msg;

} Pipeline;

bool create_pipeline(Pipeline *pipe);

bool update_pipeline_state(Pipeline *pipe);

bool check_pipeline_for_message(Pipeline *pipe);

bool cleanup_pipeline(Pipeline *pipe);

#endif