#include <gst/gst.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

int tutorial_main(int argc, char *argv[])
{
    GstElement *pipeline, *source, *sink, *filter, *videoconvert;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    // 创建新元素(元素类型，实例名称)
    // videotestsrc：源元素，生成数据，常用于调试
    source = gst_element_factory_make("videotestsrc", "source");
    // 接收器元素：视频过滤
    filter = gst_element_factory_make("vertigotv", "filter");
    // 
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    // autovideosink：接收器元素，消费数据，窗口显示接收到的图像
    sink = gst_element_factory_make("autovideosink", "sink");

    /* Create the empty pipeline */
    // 创建一个新的管道，所有的元素必须包含在管道内才能使用，因为它负责一些时钟和消息传递功能
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !source || !sink || !filter || !videoconvert)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    /* 管道的基类为 GstBin 它是用于包含其他元素的元素
       此处将元素添加到管道中
       可以使用gst_bin_add ()添加单个元素
    */
    gst_bin_add_many(GST_BIN(pipeline), source, filter, videoconvert, sink, NULL);
    // 关联元素，顺序按照数据流，只有驻留在同一容器中的元素才能链接在一起
    if (gst_element_link(source, filter) != TRUE)
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_element_link(filter, videoconvert);
    gst_element_link(videoconvert, sink);
    
    // if(gst_element_link_many(source, filter, videoconvert, sink) != TRUE)
    // {
    //     g_printerr("Elements could not be linked.\n");
    //     gst_object_unref(pipeline);
    //     return -1;
    // }

    /* Modify the source's properties */
    // 元素通过GObject提供属性设置
    // 属性通过g_object_get ()读取并通过g_object_set ()写入
    // 更改了videotestsrc的“pattern”属性，该属性控制元素输出的测试视频的类型。尝试不同的值
    g_object_set(source, "pattern", 0, NULL);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        // GstMessage是一种非常通用的结构，几乎可以传递任何类型的信息
        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                       GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n",
                       debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(msg);
    }

    /* Free resources */
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