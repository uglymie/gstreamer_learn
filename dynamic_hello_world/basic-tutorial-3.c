#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it to callbacks */
// 将所有数据分组到一个结构中以便于处理
typedef struct _CustomData
{
    GstElement *pipeline;
    GstElement *source;
    GstElement *convert;
    GstElement *resample;
    GstElement *sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler(GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[])
{
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    // uridecodebin将在内部实例化所有必要的元素（源、解复用器和解码器），
    // 以将 URI 转换为原始音频和/或视频流。它完成了一半的工作playbin。
    // 由于它包含解复用器，因此它的源焊盘最初不可用，我们需要动态链接到它们
    data.source = gst_element_factory_make("uridecodebin", "source");
    // audioconvert对于不同音频格式之间的转换非常有用，确保此示例可以在任何平台上运行，
    // 因为音频解码器生成的格式可能与音频接收器期望的格式不同。
    data.convert = gst_element_factory_make("audioconvert", "convert");
    // audioresample对于在不同音频采样率之间进行转换非常有用，同样确保此示例可以在任何平台上工作，
    // 因为音频解码器产生的音频采样率可能不是音频接收器支持的采样率。
    data.resample = gst_element_factory_make("audioresample", "resample");
    // 将音频流渲染到声卡
    data.sink = gst_element_factory_make("autoaudiosink", "sink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.pipeline || !data.source || !data.convert || !data.resample || !data.sink)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline. Note that we are NOT linking the source at this
     * point. We will do it later. */
    // 暂时不将它们与源链接，因为此时它不包含源焊盘
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.resample, data.sink, NULL);
    if (!gst_element_link_many(data.convert, data.resample, data.sink, NULL))
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Set the URI to play */
    // 通过属性设置要播放的文件的 URI
    g_object_set(data.source, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    /* Connect to the pad-added signal */
    // GSignals是 GStreamer 中的一个关键点。当发生有趣的事情时，它们允许您收到通知（通过回调）
    // 信号由名称来标识，每个信号GObject都有自己的信号
    g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.pipeline);
    do
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                         GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        /* Parse message */
        if (msg != NULL)
        {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE(msg))
            {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                terminate = TRUE;
                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline))
                {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    g_print("Pipeline state changed from %s to %s:\n",
                            gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                }
                break;
            default:
                /* We should not reach here */
                g_printerr("Unexpected message received.\n");
                break;
            }
            gst_message_unref(msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

/* This function will be called by the pad-added signal */
// 有源数据时触发，添加pad
static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data)
{
    GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* If our converter is already linked, we have nothing to do here */
    // 一旦已经链接，将阻止尝试链接到新的 pad
    if (gst_pad_is_linked(sink_pad))
    {
        g_print("We are already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    // 检查这个新 pad 将输出的数据类型
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    // 检索第一个，代指音频
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    // 获取结构的名称，实际上是其媒体类型
    new_pad_type = gst_structure_get_name(new_pad_struct);
    // 如果名称不是audio/x-raw，则这不是解码的音频
    if (!g_str_has_prefix(new_pad_type, "audio/x-raw"))
    {
        g_print("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    // 尝试连接两个 pad，和 gst_element_link() 一样
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        g_print("Type is '%s' but link failed.\n", new_pad_type);
    }
    else
    {
        g_print("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref(new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref(sink_pad);
}