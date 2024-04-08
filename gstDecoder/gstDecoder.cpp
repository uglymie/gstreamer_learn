#include "gstDecoder.h"
#include <gst/app/gstappsink.h>
#include <sstream> 

#include <string.h>
#include <math.h>


// constructor
gstDecoder::gstDecoder()
{	
	mAppSink   = NULL;
	mBus       = NULL;
	mPipeline  = NULL;
}


// destructor	
gstDecoder::~gstDecoder()
{
	Close();

	if( mAppSink != NULL )
	{
		gst_object_unref(mAppSink);
		mAppSink = NULL;
	}

	if( mBus != NULL )
	{
		gst_object_unref(mBus);
		mBus = NULL;
	}

	if( mPipeline != NULL )
	{
		gst_object_unref(mPipeline);
		mPipeline = NULL;
	}
	
	// SAFE_DELETE(mBufferManager);
}

// Create
gstDecoder* gstDecoder::Create( )
{
	// create camera instance
	gstDecoder* cam = new gstDecoder();
	
	if( !cam )
		return NULL;
	
	// initialize camera (with fallback)
	if( !cam->init() )
	{
		// printf("gstDecoder -- failed to create device %s\n", cam->GetResource().c_str());
		return NULL;
	}
	
	// printf("gstDecoder successfully created device %s\n", cam->GetResource().c_str()); 
	return cam;
}

// pick the closest framerate
float gstDecoder::findFramerate( const std::vector<float>& frameRates, float frameRate ) const
{
	const uint32_t numRates = frameRates.size();
	
	if( numRates == 0 )
		return frameRate;
	
	float bestRate = 0.0f;
	float bestDiff = 10000.0f;
	
	for( uint32_t n=0; n < numRates; n++ )
	{
		const float diff = fabsf(frameRates[n] - frameRate);
		
		if( diff < bestDiff )
		{
			bestRate = frameRates[n];
			bestDiff = diff;
		}
	}
	
	return bestRate;
}

// init
bool gstDecoder::init()
{
	// 初始化
	int argc = 0;
	if( !gst_init_check(&argc, NULL, NULL) )
	{
		printf("failed to initialize gstreamer library with gst_init()\n");
		return false;
	}

	// 获取版本信息
	const gchar *nano_str;
  	guint major, minor, micro, nano;
	gst_version (&major, &minor, &micro, &nano);

	if (nano == 1)
    	nano_str = "(CVS)";
  	else if (nano == 2)
    	nano_str = "(Prerelease)";
  	else
    	nano_str = "";

  	printf ("This program is linked against GStreamer %d.%d.%d %s\n",
          	major, minor, micro, nano_str);

	// 解析并启动 uri
	GError* err = NULL;
	// std::string uri = "nvarguscamerasrc sensor-id=0 ! video/x-raw(memory:NVMM), width=(int)1280, height=(int)720, framerate=30/1, format=(string)NV12 ! nvvidconv flip-method=2 ! video/x-raw ! appsink name=mysink";
	// std::string uri = "filesrc location=D://video/sample.mp4 ! qtdemux ! queue ! h264parse ! nvv4l2decoder name=decoder enable-max-performance=1 ! video/x-raw(memory:NVMM) ! nvvidconv name=vidconv ! video/x-raw ! appsink name=mysink";
	// std::string uri = "filesrc location=D:/video/sample.mp4 ! qtdemux ! queue ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=NV12 ! appsink name=mysink";
	std::string uri = "rtspsrc location=rtsp://192.168.2.160/livestream/12 ! rtph264depay ! h264parse ! avdec_h264 name=decoder ! queue ! videoconvert ! video/x-raw,format=(string)NV12 ! appsink name=mysink sync=false";
	// Windows 下 d3d11h264dec 解码器比 openh264dec 快很多 avdec_h264 也很慢
	// 不要直接在管道中转码为RGB，可以转为NV12，否则会比较慢
	// launch pipeline
	mPipeline = gst_parse_launch(uri.c_str(), &err);

	if( err != NULL )
	{
		printf("gstDecoder failed to create pipeline\n");
		printf("   (%s)\n", err->message);
		g_error_free(err);
		return false;
	}

	GstPipeline* pipeline = GST_PIPELINE(mPipeline);

	if( !pipeline )
	{
		printf("gstDecoder failed to cast GstElement into GstPipeline\n");
		return false;
	}	

	// retrieve pipeline bus
	mBus = gst_pipeline_get_bus(pipeline);

	if( !mBus )
	{
		printf("gstDecoder failed to retrieve GstBus from pipeline\n");
		return false;
	}

	// add watch for messages (disabled when we poll the bus ourselves, instead of gmainloop)
	//gst_bus_add_watch(mBus, (GstBusFunc)gst_message_print, NULL);

	// get the appsrc 用于接收 GStreamer 流中的数据
	GstElement* appsinkElement = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
	GstAppSink* appsink = GST_APP_SINK(appsinkElement);

	if( !appsinkElement || !appsink)
	{
		printf("gstDecoder failed to retrieve AppSink element from pipeline\n");
		return false;
	}
	
	mAppSink = appsink;
	
	// setup callbacks
	GstAppSinkCallbacks cb;
	memset(&cb, 0, sizeof(GstAppSinkCallbacks));
	
	cb.eos         = onEOS;		// 回调函数 onEOS 会在流结束时被调用
	cb.new_preroll = onPreroll;	// 回调函数 onPreroll 会在新 Preroll 数据可用时被调用
	cb.new_sample  = onBuffer;	// 回调函数 onBuffer 会在新的样本数据可用时被调用。这是主要用于处理每一帧数据的回调。
	
	gst_app_sink_set_callbacks(mAppSink, &cb, (void*)this, NULL);
	
	// disable looping for cameras
	// mOptions.loop = 0;	// 防止在相机应用中无限循环播放/

	return true;
}


// onEOS
void gstDecoder::onEOS(_GstAppSink* sink, void* user_data)
{
	printf("gstDecoder -- end of stream (EOS)\n");
	cv::destroyAllWindows();
	if( !user_data )
		return;

	// gstDecoder* dec = (gstDecoder*)user_data;

	// dec->mEOS = true;	
	// dec->mStreaming = dec->isLooping();
}

// onPreroll
GstFlowReturn gstDecoder::onPreroll(_GstAppSink* sink, void* user_data)
{
	printf("gstDecoder -- onPreroll\n");

	if( !user_data )
		return GST_FLOW_OK;

	gstDecoder* dec = (gstDecoder*)user_data;
	dec->checkMsgBus();
	return GST_FLOW_OK;
}

// onBuffer
GstFlowReturn gstDecoder::onBuffer(_GstAppSink* sink, void* user_data)
{
	// printf( "gstDecoder onBuffer\n");
	
	if( !user_data )
		return GST_FLOW_OK;
		
	gstDecoder* dec = (gstDecoder*)user_data;
	
	dec->checkBuffer();
	dec->checkMsgBus();
	
	return GST_FLOW_OK;
}
	

#define release_return { gst_sample_unref(gstSample); return; }

// checkBuffer
void gstDecoder::checkBuffer()
{
	if( !mAppSink )
		return;

	// 等待缓冲区的块
	GstSample* gstSample = gst_app_sink_pull_sample(mAppSink);
	
	if( !gstSample )
	{
		printf("gstDecoder -- app_sink_pull_sample() returned NULL...\n");
		return;
	}
	
	// 获取样本的容器格式（caps）表示该样本包含的多媒体数据的描述
	GstCaps* gstCaps = gst_sample_get_caps(gstSample);
	
	if( !gstCaps )
	{
		printf("gstDecoder -- gst_sample had NULL caps...\n");
		release_return;
	}
	GstStructure* gstCapsStruct = gst_caps_get_structure(gstCaps, 0);
	const char* format = gst_structure_get_string(gstCapsStruct, "format");
	int width, height;
	gst_structure_get_int(gstCapsStruct, "width", &width);
	gst_structure_get_int(gstCapsStruct, "height", &height);
	
	
	// 从样本中检索缓冲区，缓冲区包含实际的多媒体数据
	GstBuffer* gstBuffer = gst_sample_get_buffer(gstSample);
	
	if( !gstBuffer )
	{
		printf("gstDecoder -- app_sink_pull_sample() returned NULL...\n");
		release_return;
	}

	GstMapInfo mapInfo;
	gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_READ);
	// format is NV12, buffer size: 12441600, width: 3840, height: 2160
	printf("format is %s, buffer size: %d, width: %d, height: %d\n", format, mapInfo.size, width, height);
	
	cv::Mat nv12Mat(height * 3 / 2, width, CV_8UC1, (void*)mapInfo.data);
    cv::Mat bgrMat(height, width, CV_8UC3);
    cv::cvtColor(nv12Mat, bgrMat, cv::COLOR_YUV2BGR_NV12);
	cv::resize(bgrMat, bgrMat, cv::Size(1280, 720));
	cv::imshow("sample", bgrMat);
	
	// cv::Mat frame(720, 1280, CV_8UC3, mapInfo.data);
	// cv::imshow("sample", frame);
	// cv::imwrite("sample.jpg", frame);
	cv::waitKey(1);

	gst_buffer_unmap(gstBuffer, &mapInfo);
	// // enqueue the buffer for color conversion
	// if( !mBufferManager->Enqueue(gstBuffer, gstCaps) )
	// {
	// 	printf("gstDecoder -- failed to handle incoming buffer\n");
	// 	release_return;
	// }
	
	// mOptions.frameCount++;
	release_return;
}


#define RETURN_STATUS(code)  { if( status != NULL ) { *status=(code); } return ((code) == videoSource::OK ? true : false); }


// Capture
bool gstDecoder::Capture( void** output, int* status )
{
	// verify the output pointer exists
	if( !output )
		return false;
	if( !Open() )
		return false;


	// wait until a new frame is recieved
	// const int result = mBufferManager->Dequeue(output, format, timeout);
	
	// if( result < 0 )
	// {
	// 	printf("gstDecoder::Capture() -- an error occurred retrieving the next image buffer\n");
	// 	RETURN_STATUS(ERROR);
	// }
	// else if( result == 0 )
	// {
	// 	LogWarning(LOG_GSTREAMER "gstDecoder::Capture() -- a timeout occurred waiting for the next image buffer\n");
	// 	RETURN_STATUS(TIMEOUT);
	// }

	// mLastTimestamp = mBufferManager->GetLastTimestamp();
	// mRawFormat = mBufferManager->GetRawFormat();

	// RETURN_STATUS(OK);
	return true;
}

// Open
bool gstDecoder::Open()
{
	// if (gst_element_set_state(mPipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
	// 	g_print("Failed to set pipeline state to PAUSED\n");
	// 	return false;
	// }

	// const bool seek = gst_element_seek(mPipeline, 1.0, GST_FORMAT_TIME,
	// 								(GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
	// 								GST_SEEK_TYPE_SET, 0LL,
	// 								GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE );

	// if( !seek )
	// {
	// 	printf("gstDecoder -- failed to seek stream to beginning (loop %zu of %i)\n");
	// 	return false;
	// }

	// transition pipline to STATE_PLAYING
	printf("opening gstDecoder for streaming, transitioning pipeline to GST_STATE_PLAYING\n");
	
	const GstStateChangeReturn result = gst_element_set_state(mPipeline, GST_STATE_PLAYING);

	if( result == GST_STATE_CHANGE_ASYNC )
	{
#if 0
		GstMessage* asyncMsg = gst_bus_timed_pop_filtered(mBus, 5 * GST_SECOND, 
    	 					      (GstMessageType)(GST_MESSAGE_ASYNC_DONE|GST_MESSAGE_ERROR)); 

		if( asyncMsg != NULL )
		{
			gst_message_print(mBus, asyncMsg, this);
			gst_message_unref(asyncMsg);
		}
		else
			printf(LOG_GSTREAMER "gstDecoder NULL message after transitioning pipeline to PLAYING...\n");
#endif
	}
	else if( result != GST_STATE_CHANGE_SUCCESS )
	{
		printf("gstDecoder failed to set pipeline state to PLAYING (error %u)\n", result);
		return false;
	}

	checkMsgBus();
	_sleep(100);
	checkMsgBus();

	// mStreaming = true;
	return true;
}
	
// Close
void gstDecoder::Close()
{
	const GstStateChangeReturn result = gst_element_set_state(mPipeline, GST_STATE_NULL);

	if( result != GST_STATE_CHANGE_SUCCESS )
		printf("gstDecoder failed to set pipeline state to PLAYING (error %u)\n", result);

	// usleep(250*1000);	
	_sleep(250);
	checkMsgBus();
	// mStreaming = false;
	// LogInfo(LOG_GSTREAMER "gstDecoder -- pipeline stopped\n");
}

// checkMsgBus
void gstDecoder::checkMsgBus()
{
	while(true)
	{
		GstMessage* msg = gst_bus_pop(mBus);

		if( !msg )
			break;

		// gst_message_print(mBus, msg, this);
		gst_message_unref(msg);
	}
}

