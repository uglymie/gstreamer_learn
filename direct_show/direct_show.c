#include <gst/gst.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    const gchar *uri = "dshowvideosrc device-index=0 ! autovideosink";
    GstElement *pipeline = gst_parse_launch(uri, NULL);

    if (!pipeline) {
        g_printerr("Failed to create pipeline. Exiting.\n");
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
