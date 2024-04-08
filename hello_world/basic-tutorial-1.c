#include <gst/gst.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

int tutorial_main(int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    /* Initialize GStreamer */
    gst_init(&argc, &argv); // 初始化所有内部结构

    /* Build the pipeline */
    pipeline =
        gst_parse_launch                                                                      // 文本快捷构建管道
        /* 构建一个由名为playbin的单个元素组成的管道
           传递要播放媒体的 URL 
           可以更改为其他内容，https:// file://
        */
        // ("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", 
        // ("playbin uri=file:///d:/video/sample.mp4",
        ("rtsp://192.168.2.160/livestream/12",
         NULL);

    // GstPipeline* mpipeline = GST_PIPELINE(pipeline);
    /* Start playing */
    // 将管道状态设为播放
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    

    /* Wait until error or EOS */
    // 检索管道
    bus = gst_element_get_bus(pipeline);
    // 阻塞，直到结束或者出现错误
    msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* See next tutorial for proper error message handling/parsing */
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
    {
        g_error("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
                "variable set for more details.");
    }

    /* Free resources */
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

int main(int argc, char *argv[])
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
    return gst_macos_main(tutorial_main, argc, argv, NULL);
#else
    return tutorial_main(argc, argv);
#endif
}