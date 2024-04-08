#include "gstCamera.h"
#include <gst/app/gstappsink.h>
#include <sstream> 

#include <string.h>
#include <math.h>


// constructor
gstCamera::gstCamera()
{	
	mAppSink   = NULL;
	mBus       = NULL;
	mPipeline  = NULL;	
}


// destructor	
gstCamera::~gstCamera()
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
gstCamera* gstCamera::Create( )
{
	// create camera instance
	gstCamera* cam = new gstCamera();
	
	if( !cam )
		return NULL;
	
	// initialize camera (with fallback)
	if( !cam->init() )
	{
		// printf("gstCamera -- failed to create device %s\n", cam->GetResource().c_str());
		return NULL;
	}
	
	// printf("gstCamera successfully created device %s\n", cam->GetResource().c_str()); 
	return cam;
}

// pick the closest framerate
float gstCamera::findFramerate( const std::vector<float>& frameRates, float frameRate ) const
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

bool gstCamera::discover()
{
	// 创建v4l2设备服务
	GstDeviceProvider* deviceProvider = gst_device_provider_factory_get_by_name("v4l2deviceprovider");
	
	if( !deviceProvider )
	{
		printf( "gstCamera -- failed to create v4l2 device provider during discovery\n");
		return false;
	}
	
	// 获取v4l2设备的列表
	GList* deviceList = gst_device_provider_get_devices(deviceProvider);

	if( !deviceList )
	{
		printf( "gstCamera -- didn't discover any v4l2 devices\n");
		return false;
	}

	// 查找请求的/dev/video*设备
	GstDevice* device = NULL;
	
	for( GList* n=deviceList; n; n = n->next )
	{
		GstDevice* d = GST_DEVICE(n->data);
		
		const char* deviceName = gst_device_get_display_name(d);
		
		printf( "gstCamera -- found v4l2 device: %s\n", deviceName);
	
	#if NV_TENSORRT_MAJOR > 8 || (NV_TENSORRT_MAJOR == 8 && NV_TENSORRT_MINOR >= 4)
		// on JetPack >= 5.0.1, the newer Logitech C920's send a H264 stream that nvv4l2decoder has trouble decoding, so change it to MJPEG
		if( strcmp(deviceName, "HD Pro Webcam C920") == 0 && mOptions.codecType == videoOptions::CODEC_V4L2 && mOptions.codec == videoOptions::CODEC_UNKNOWN )
			mOptions.codec = videoOptions::CODEC_MJPEG;
	#endif
	
		GstStructure* properties = gst_device_get_properties(d);
		
		if( properties != NULL )
		{
			printf( "%s\n", gst_structure_to_string(properties));
			
			const char* devicePath = gst_structure_get_string(properties, "device.path");
			
			// if( devicePath != NULL && strcasecmp(devicePath, mOptions.resource.location.c_str()) == 0 )
			// {
			// 	device = d;
			// 	break;
			// }
		}
	}
	
	if( !device )
	{
		// printf( "gstCamera -- could not find v4l2 device %s\n", mOptions.resource.location.c_str());
		return false;
	}
	
	// get the caps of the device
	GstCaps* device_caps = gst_device_get_caps(device);
	
	if( !device_caps )
	{
		// printf( "gstCamera -- failed to retrieve caps for v4l2 device %s\n", mOptions.resource.location.c_str());
		return false;
	}
	
	// printCaps(device_caps);
	
	// pick the best caps
	// if( !matchCaps(device_caps) )
	// 	return false;
	
	// printf( "gstCamera -- selected device profile:  codec=%s format=%s width=%u height=%u framerate=%u\n", videoOptions::CodecToStr(mOptions.codec), imageFormatToStr(mFormatYUV), GetWidth(), GetHeight(), GetFrameRate());
	
	return true;
}

// init
bool gstCamera::init()
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

	// discover();

	// 解析并启动 uri
	GError* err = NULL;
	// std::string uri = "nvarguscamerasrc sensor-id=0 ! video/x-raw(memory:NVMM), width=(int)1280, height=(int)720, framerate=30/1, format=(string)NV12 ! nvvidconv flip-method=2 ! video/x-raw ! appsink name=mysink";
	// std::string uri = "filesrc location=D://video/sample.mp4 ! qtdemux ! queue ! h264parse ! nvv4l2decoder name=decoder enable-max-performance=1 ! video/x-raw(memory:NVMM) ! nvvidconv name=vidconv ! video/x-raw ! appsink name=mysink";
	std::string uri = "dshowvideosrc device-index=0 do-timestamp=true ! videoconvert ! video/x-raw, format=NV12, width=(int)1280, height=(int)720 ! appsink name=mysink sync=false";
	// std::string uri = "dshowvideosrc device-index=0 do-timestamp=true ! nvv4l2decoder ! nvvidconv ! video/x-raw,format=BGR,width=(int)1280,height=(int)720 ! appsink name=mysink sync=false";

	// launch pipeline
	mPipeline = gst_parse_launch(uri.c_str(), &err);

	if( err != NULL )
	{
		printf("gstCamera failed to create pipeline\n");
		printf("   (%s)\n", err->message);
		g_error_free(err);
		return false;
	}

	GstPipeline* pipeline = GST_PIPELINE(mPipeline);

	if( !pipeline )
	{
		printf("gstCamera failed to cast GstElement into GstPipeline\n");
		return false;
	}	

	// retrieve pipeline bus
	mBus = gst_pipeline_get_bus(pipeline);

	if( !mBus )
	{
		printf("gstCamera failed to retrieve GstBus from pipeline\n");
		return false;
	}

	// add watch for messages (disabled when we poll the bus ourselves, instead of gmainloop)
	//gst_bus_add_watch(mBus, (GstBusFunc)gst_message_print, NULL);

	// get the appsrc 用于接收 GStreamer 流中的数据
	GstElement* appsinkElement = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
	GstAppSink* appsink = GST_APP_SINK(appsinkElement);

	if( !appsinkElement || !appsink)
	{
		printf("gstCamera failed to retrieve AppSink element from pipeline\n");
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
void gstCamera::onEOS(_GstAppSink* sink, void* user_data)
{
	printf("gstCamera -- end of stream (EOS)\n");
	cv::destroyAllWindows();
	if( !user_data )
		return;

	// gstCamera* dec = (gstCamera*)user_data;

	// dec->mEOS = true;	
	// dec->mStreaming = dec->isLooping();
}

// onPreroll
GstFlowReturn gstCamera::onPreroll(_GstAppSink* sink, void* user_data)
{
	printf("gstCamera -- onPreroll\n");

	if( !user_data )
		return GST_FLOW_OK;

	gstCamera* dec = (gstCamera*)user_data;
	dec->checkMsgBus();
	return GST_FLOW_OK;
}

// onBuffer
GstFlowReturn gstCamera::onBuffer(_GstAppSink* sink, void* user_data)
{
	printf( "gstCamera onBuffer\n");
	
	if( !user_data )
		return GST_FLOW_OK;
		
	gstCamera* dec = (gstCamera*)user_data;
	
	dec->checkBuffer();
	dec->checkMsgBus();
	
	return GST_FLOW_OK;
}
	

#define release_return { gst_sample_unref(gstSample); return; }

// checkBuffer
void gstCamera::checkBuffer()
{
	if( !mAppSink )
		return;

	// 等待缓冲区的块
	GstSample* gstSample = gst_app_sink_pull_sample(mAppSink);
	
	if( !gstSample )
	{
		printf("gstCamera -- app_sink_pull_sample() returned NULL...\n");
		return;
	}
	
	// 获取样本的容器格式（caps）表示该样本包含的多媒体数据的描述
	GstCaps* gstCaps = gst_sample_get_caps(gstSample);
	
	if( !gstCaps )
	{
		printf("gstCamera -- gst_sample had NULL caps...\n");
		release_return;
	}
	
	// 从样本中检索缓冲区，缓冲区包含实际的多媒体数据
	GstBuffer* gstBuffer = gst_sample_get_buffer(gstSample);
	
	if( !gstBuffer )
	{
		printf("gstCamera -- app_sink_pull_sample() returned NULL...\n");
		release_return;
	}

	GstMapInfo mapInfo;
	gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_READ);

	// for (int i = 0; i < 10; ++i) {
	// 	for (int j = 0; j < 10; ++j) {
	// 		std::cout << static_cast<int>(mapInfo.data[i * 10 + j]) << ",";
	// 	}
	// 	std::cout << std::endl;
	// }

	int height = 720;
	int width = 1280;
	cv::Mat nv12Mat(height * 3 / 2, width, CV_8UC1, (void*)mapInfo.data);
    cv::Mat bgrMat(height, width, CV_8UC3);
    cv::cvtColor(nv12Mat, bgrMat, cv::COLOR_YUV2BGR_NV12);

	// cv::Mat frame(720, 1280, CV_8UC3, mapInfo.data);
	// cv::Mat frame(1080, 1920, CV_8UC3, mapInfo.data);
	// cv::normalize(frame, frame, 0, 255, cv::NORM_MINMAX);
	cv::imshow("sample", bgrMat);
	// cv::imwrite("sample.jpg", frame);
	cv::waitKey(1);

	gst_buffer_unmap(gstBuffer, &mapInfo);
	// // enqueue the buffer for color conversion
	// if( !mBufferManager->Enqueue(gstBuffer, gstCaps) )
	// {
	// 	printf("gstCamera -- failed to handle incoming buffer\n");
	// 	release_return;
	// }
	
	// mOptions.frameCount++;
	release_return;
}


#define RETURN_STATUS(code)  { if( status != NULL ) { *status=(code); } return ((code) == videoSource::OK ? true : false); }


// Capture
bool gstCamera::Capture( void** output, int* status )
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
	// 	printf("gstCamera::Capture() -- an error occurred retrieving the next image buffer\n");
	// 	RETURN_STATUS(ERROR);
	// }
	// else if( result == 0 )
	// {
	// 	LogWarning(LOG_GSTREAMER "gstCamera::Capture() -- a timeout occurred waiting for the next image buffer\n");
	// 	RETURN_STATUS(TIMEOUT);
	// }

	// mLastTimestamp = mBufferManager->GetLastTimestamp();
	// mRawFormat = mBufferManager->GetRawFormat();

	// RETURN_STATUS(OK);
	return true;
}

// Open
bool gstCamera::Open()
{
	if (gst_element_set_state(mPipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
		g_print("Failed to set pipeline state to PAUSED\n");
		return false;
	}

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
	printf("opening gstCamera for streaming, transitioning pipeline to GST_STATE_PLAYING\n");
	
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
			printf(LOG_GSTREAMER "gstCamera NULL message after transitioning pipeline to PLAYING...\n");
#endif
	}
	else if( result != GST_STATE_CHANGE_SUCCESS )
	{
		printf("gstCamera failed to set pipeline state to PLAYING (error %u)\n", result);
		return false;
	}

	checkMsgBus();
	_sleep(100);
	checkMsgBus();

	// mStreaming = true;
	return true;
}
	
// Close
void gstCamera::Close()
{
	const GstStateChangeReturn result = gst_element_set_state(mPipeline, GST_STATE_NULL);

	if( result != GST_STATE_CHANGE_SUCCESS )
		printf("gstCamera failed to set pipeline state to PLAYING (error %u)\n", result);

	// usleep(250*1000);	
	_sleep(250);
	checkMsgBus();
	// mStreaming = false;
	// LogInfo(LOG_GSTREAMER "gstCamera -- pipeline stopped\n");
}

// checkMsgBus
void gstCamera::checkMsgBus()
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

