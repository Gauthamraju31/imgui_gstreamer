#include "pipeline.h"

/* This function will be called by the pad-added signal */
static void pad_added_handler(GstElement *src, GstPad *new_pad, Pipeline *data)
{
  GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked(sink_pad))
  {
    g_print("  We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps(new_pad);
  new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  new_pad_type = gst_structure_get_name(new_pad_struct);
  if (!g_str_has_prefix(new_pad_type, "video/x-raw"))
  {
    g_print("  It has type '%s' which is not raw video. Ignoring.\n", new_pad_type);
    goto exit;
  }

  /* Attempt the link */
  ret = gst_pad_link(new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED(ret))
  {
    g_print("  Type is '%s' but link failed.\n", new_pad_type);
  }
  else
  {
    g_print("  Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref(new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref(sink_pad);
}

bool create_pipeline(Pipeline *pipe)
{
  strcpy(pipe->stream_uri,"http://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm");
  // strcpy(pipe->stream_uri,"file://sample.mp4");
  pipe->loop = true;
  pipe->pipeline = gst_pipeline_new("imgui_pipeline");
  pipe->videosrc = gst_element_factory_make("uridecodebin", "videosrc0");
  pipe->convert = gst_element_factory_make("videoconvert", "videoconvert0");
  pipe->scale = gst_element_factory_make("videoscale", "videoscale0");
  pipe->appsink = gst_element_factory_make("appsink", "appsink0");

  g_object_set(pipe->videosrc, "uri", pipe->stream_uri, NULL);
  gst_bin_add_many(GST_BIN(pipe->pipeline), pipe->videosrc, pipe->convert, pipe->scale, pipe->appsink, NULL);

  g_signal_connect(pipe->videosrc, "pad-added", G_CALLBACK(pad_added_handler), pipe);

  GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                      "format", G_TYPE_STRING, "RGBA",
                                      "width", G_TYPE_INT, 1280,
                                      "height", G_TYPE_INT, 720,
                                      // "framerate", GST_TYPE_FRACTION, 10, 1,
                                      NULL);

  if (!pipe->pipeline || !pipe->videosrc || !pipe->convert || !pipe->scale || !pipe->appsink)
  {
    g_printerr("Not all elements could be created.\n");
    return false;
  }

  gboolean link_ok = gst_element_link(pipe->convert, pipe->scale);
  g_assert(link_ok);

  link_ok = gst_element_link_filtered(pipe->scale, pipe->appsink, caps);
  g_assert(link_ok);
  gst_caps_unref(caps);

  gst_element_set_state(pipe->pipeline, GST_STATE_READY);
  g_print("Ready State\n");

  pipe->bus = gst_element_get_bus(pipe->pipeline);

  pipe->pipeState = GST_STATE_PLAYING;
  gst_element_set_state(pipe->pipeline, GST_STATE_PLAYING);
  g_print("Play State\n");

  return true;
}

bool update_pipeline_state(Pipeline *pipe)
{
  //Get current state
  GstState state;
  gst_element_get_state(pipe->pipeline, &state, NULL, 0);
  if (state != pipe->pipeState)
  {
    g_object_set(pipe->videosrc, "uri", pipe->stream_uri, NULL);
    gst_element_set_state(pipe->pipeline, pipe->pipeState);
  }

  return true;
}

bool check_pipeline_for_message(Pipeline *pipe)
{
  pipe->msg = gst_bus_pop_filtered(pipe->bus,
                                   static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  if (pipe->msg != nullptr)
  {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(pipe->msg))
    {

    case GST_MESSAGE_ERROR:
      gst_message_parse_error(pipe->msg, &err, &debug_info);
      g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(pipe->msg->src), err->message);
      g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error(&err);
      g_free(debug_info);
      break;
    case GST_MESSAGE_EOS:
      g_print("End-Of-Stream reached.\n");
      if (pipe->loop)
      {
        if (!gst_element_seek(pipe->videosrc,
                              1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                              GST_SEEK_TYPE_SET, 2000000000, //2 seconds (in nanoseconds)
                              GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        {
          g_print("Seek failed!\n");
        }
      }
      break;
    default:
      g_printerr("Unexpected message received.\n");
      break;
    }
  }
  // gst_message_unref(pipe->msg);
  return true;
}

bool cleanup_pipeline(Pipeline *pipe)
{
  // gst_message_unref (pipe->msg);
  gst_object_unref(pipe->bus);
  gst_element_set_state(pipe->pipeline, GST_STATE_NULL);
  gst_object_unref(pipe->pipeline);
  // gst_object_unref (pipe->videosrc);
  // gst_object_unref (pipe->appsink);
  return true;
}

