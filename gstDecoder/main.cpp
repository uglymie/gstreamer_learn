#include "gstDecoder.h"
#include <cuda_runtime.h>
#include <cuda.h>
#include <glib.h>

int main(int argc, char *argv[])
{
    gstDecoder *src = NULL;
    src = gstDecoder::Create();

    src->Open();
    while( 1 )
    {
        
    // 	uchar3* image = NULL;
    // 	int status = 0;
    // 	src->Capture(&image, &status);
    }
}