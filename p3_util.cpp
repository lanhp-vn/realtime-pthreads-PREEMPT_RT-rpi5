#include "p3_util.h"

void BusyCal() {
    long abc = 1e9;
    while (abc--) {
        long tmp = 10000 * 10000;
    }
}

void CannyP3() {
    char outfilename[128];
    unsigned char *image;
    unsigned char *edge;
    int rows, cols;
    float sigma, tlow, thigh;
    int cnt = 0;

    sigma = 1;
    tlow = 0.2;
    thigh = 0.6;
    rows = HEIGHT;
    cols = WIDTH;

    VideoCapture cap;
    // open the default camera (/dev/video0)
    // Check VideoCapture documentation for more details
    // if(!cap.open(0)){
    if (!cap.open("ground_crew_480p.mp4")) {
        cout << "Failed to open /dev/video0" << endl;
        exit(0);
        // return NULL;
    }
    cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
    cap.set(CAP_PROP_FRAME_HEIGHT, HEIGHT);

    Mat frame, grayframe;
    // test capture
    cap >> frame;

#if IMSHOW_DISPLAY
    printf("[INFO] (On the pop-up window) Press ESC to terminate the program...\n");
#endif
    printf("Writing the edge image in the file: ");
    for (;;) {
        cap >> frame;
        if (frame.empty()) {
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap >> frame;
        }  // end of video stream, repeat
        cvtColor(frame, grayframe, COLOR_BGR2GRAY);
        image = grayframe.data;
        canny(image, rows, cols, sigma, tlow, thigh, &edge, NULL);
        sprintf(outfilename, "camera_s_%3.2f_l_%3.2f_h_%3.2f_%d.pgm", sigma, tlow, thigh, cnt++);
        if (write_pgm_image(outfilename, edge, rows, cols, NULL, 255) == 0) {
            fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
            exit(1);
        }
        printf(">");
#if IMSHOW_DISPLAY
        grayframe.data = edge;
        imshow("[EDGE] this is you, smile! :)", grayframe);
        if (waitKey(10) == 27) break;  // stop capturing by pressing ESC
#endif

        if (cnt >= MAX_FRAME_NUM) break;  // AUTO-OFF after 100 frames
    }
    printf("\n");
}
