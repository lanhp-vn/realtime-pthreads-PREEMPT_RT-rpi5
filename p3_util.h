/**
 */
#include <unistd.h>

#include <iostream>

#include "canny_util.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

/* Possible options: 320x240, 640x480, 1024x768, 1280x1040, and so on. */
/* Pi Camera MAX resolution: 2592x1944 */

#define WIDTH 854
#define HEIGHT 480

#define IMSHOW_DISPLAY false
#define MAX_FRAME_NUM 100

void BusyCal();
void CannyP3();
