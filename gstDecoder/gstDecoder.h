#ifndef __GSTREAMER_CAMERA_H__
#define __GSTREAMER_CAMERA_H__

// #include "videoSource.h"
// #include "gstBufferManager.h"

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gst/gst.h>

// Forward declarations
struct _GstAppSink;
/*
 * @see videoSource
 * @ingroup camera
 */
class gstDecoder
{
public:
	enum Status
	{
		ERROR   = -2,	/**< an error occurred */
		EOS     = -1,	/**< end-of-stream (EOS) */
		TIMEOUT = 0,	/**< a timeout occurred */
		OK      = 1	/**< frame capture successful */
	};

	/**
	 * Create a MIPI CSI or V4L2 camera device.
	 */
	static gstDecoder* Create();

	/**
	 * Release the camera interface and resources.
	 * Destroying the camera will also Close() the stream if it is still open.
	 */
	~gstDecoder();

	/**
	 * Begin streaming the camera.
	 * After Open() is called, frames from the camera will begin to be captured.
	 *
	 * Open() is not stricly necessary to call, if you call one of the Capture()
	 * functions they will first check to make sure that the stream is opened,
	 * and if not they will open it automatically for you.
	 *
	 * @returns `true` on success, `false` if an error occurred opening the stream.
	 */
	virtual bool Open();

	/**
	 * Stop streaming the camera.
	 * @note Close() is automatically called by the camera's destructor when
	 * it gets deleted, so you do not explicitly need to call Close() before
	 * exiting the program if you delete your camera object.
	 */
	virtual void Close();

	/**
	 * Capture the next image frame from the camera.
	 * @see videoSource::Capture
	 */
	virtual bool Capture( void** image, int* status=NULL );

	/**
	 * Capture the next image frame from the camera and convert it to float4 RGBA format,
	 * with pixel intensities ranging between 0.0 and 255.0.
	 *
	 * @deprecated CaptureRGBA() has been deprecated and is only provided for legacy 
	 *             compatibility. Please use the updated Capture() function instead.
	 *
	 * Internally, CaptureRGBA() first calls Capture() and then ConvertRGBA().
	 * The ConvertRGBA() function uses CUDA, so if you want to capture from a different 
	 * thread than your CUDA device, use the Capture() and ConvertRGBA() functions.
	 *
	 * @param[out] image Pointer that gets returned to the image in GPU address space,
	 *                   or if the zeroCopy parameter is true, then the pointer is valid
	 *                   in both CPU and GPU address spaces.  Do not manually free the image memory, 
	 *                   it is managed internally.  The image is in float4 RGBA format.
	 *                   The size of the image is:  `GetWidth() * GetHeight() * sizeof(float) * 4`
	 *
	 * @param[in] timeout The time in milliseconds for the calling thread to wait to
	 *                    return if a new camera frame isn't recieved by that time.
	 *                    If timeout is 0, the calling thread will return immediately
	 *                    if a new frame isn't already available.
	 *                    If timeout is UINT64_MAX, the calling thread will wait
	 *                    indefinetly for a new frame to arrive.
	 *
	 * @param[in] zeroCopy If `true`, the image will reside in shared CPU/GPU memory.
	 *                     If `false`, the image will only be accessible from the GPU.
	 *                     You would need to set zeroCopy to `true` if you wanted to
	 *                     access the image pixels from the CPU.  Since this isn't
	 *                     generally the case, the default is `false` (GPU only).
	 *
	 * @returns `true` if a frame was successfully captured, otherwise `false` if a timeout
	 *               or error occurred, or if timeout was 0 and a frame wasn't ready.
	 */
	// bool CaptureRGBA( float** image, uint64_t timeout=DEFAULT_TIMEOUT, bool zeroCopy=false );

	/**
	 * Set whether converted RGB(A) images should use ZeroCopy buffer allocation.
	 * Has no effect after the first image (in RGB(A) format) was captured.
	 */
	// void SetZeroCopy(bool zeroCopy)     { mOptions.zeroCopy = zeroCopy; }

	/**
	 * Return the interface type (gstDecoder::Type)
	 */
	virtual inline uint32_t GetType() const		{ return Type; }

	/**
	 * Unique type identifier of gstDecoder class.
	 */
	static const uint32_t Type = (1 << 0);

	/**
	 * Default camera width, unless otherwise specified during Create()
 	 */
	static const uint32_t DefaultWidth  = 1280;

	/**
	 * Default camera height, unless otherwise specified during Create()
 	 */
	static const uint32_t DefaultHeight = 720;
	
private:
	static void onEOS(_GstAppSink* sink, void* user_data);
	static GstFlowReturn onPreroll(_GstAppSink* sink, void* user_data);
	static GstFlowReturn onBuffer(_GstAppSink* sink, void* user_data);

	gstDecoder();

	bool init();

	void checkMsgBus();
	void checkBuffer();
	
	float findFramerate( const std::vector<float>& frameRates, float frameRate ) const;
	
	_GstBus*     mBus;
	_GstAppSink* mAppSink;
	_GstElement* mPipeline;

	std::string  mLaunchStr;
	// imageFormat  mFormatYUV;
	
	// gstBufferManager* mBufferManager;
};

#endif
