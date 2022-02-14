/* 

File: detectmarkers.cpp 

Author: Manuel Tolino Contreras

Decription: This file contains the code intended to generate a node which should
capture a gstreamer video input and detect ArUco markers. The detected
markers should be represented in real time in a windows superposed with
the video feed.

*/


#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstdlib>

namespace {
const char* about = "Detect ArUco marker images";
const char* keys  =
        "{d        |16    | dictionary: DICT_4X4_50=0, DICT_4X4_100=1, "
        "DICT_4X4_250=2, DICT_4X4_1000=3, DICT_5X5_50=4, DICT_5X5_100=5, "
        "DICT_5X5_250=6, DICT_5X5_1000=7, DICT_6X6_50=8, DICT_6X6_100=9, "
        "DICT_6X6_250=10, DICT_6X6_1000=11, DICT_7X7_50=12, DICT_7X7_100=13, "
        "DICT_7X7_250=14, DICT_7X7_1000=15, DICT_ARUCO_ORIGINAL = 16}"
        "{h        |false | Print help }"
        "{v        |<none>| Custom video source, otherwise '0' }"
        ;
}

int main(int argc, char **argv)
{
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about(about);

    if (parser.get<bool>("h")) {
        parser.printMessage();
        return 0;
    }

    int dictionaryId = parser.get<int>("d");
    int wait_time = 10;
    cv::String videoInput = "0";
    cv::VideoCapture in_video;

    /*Parametros copiados de inet*/
    const cv::Mat  intrinsic_matrix = (cv::Mat_<float>(3, 3)
                               << 803.9233,  0,         286.5234,
                                  0,         807.6013,  245.9685,
                                  0,         0,         1);

    const cv::Mat  distCoeffs = (cv::Mat_<float>(5, 1) << 0.1431, -0.4943, 0, 0, 0);


    const cv::Mat  arucodistCoeffs = (cv::Mat_<float>(1, 5) << 0, 0, 0, 0, 0);// La foto corregida se utiliza para la detección


    /* The output parameters rvecs and tvecs are the rotation and translation vectors respectively, for each of the markers in markerCorners.*/
    /* The markerCorners parameter is the vector of marker corners returned by the detectMarkers() function.*/
    std::vector<cv::Vec3d> rvecs, tvecs; 
    
    if (parser.has("v")) {
        videoInput = parser.get<cv::String>("v");
        if (videoInput.empty()) {
            parser.printMessage();
            return 1;
        }
        char* end = nullptr;
        int source = static_cast<int>(std::strtol(videoInput.c_str(), &end, \
            10));
        if (!end || end == videoInput.c_str()) {
            in_video.open(videoInput); // url
        } else {
            in_video.open(source); // id
            in_video.set(cv::CAP_PROP_FPS, 30);
            in_video.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            in_video.set(cv::CAP_PROP_FRAME_HEIGHT, 360);
            in_video.set(cv::CAP_PROP_SATURATION, 0);
        }
    } else {
        //in_video.open("udpsrc port=5600 ! application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink fps-update-interval=1000 sync=false ! appsink drop=1");
        in_video.open("udpsrc port=5600 ! application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264 ! rtph264depay ! avdec_h264 ! videoconvert ! appsink drop=1");
        //in_video.set(cv::CAP_PROP_FPS, 30);
        //in_video.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        //in_video.set(cv::CAP_PROP_FRAME_HEIGHT, 360);
        //in_video.set(cv::CAP_PROP_SATURATION, 0);
        printf("OPENED");
    }

    if (!parser.check()) {
        parser.printErrors();
        return 1;
    }

    if (!in_video.isOpened()) {
        std::cerr << "failed to open video input: " << videoInput << std::endl;
        return 1;
    }

    cv::Ptr<cv::aruco::Dictionary> dictionary =
        cv::aruco::getPredefinedDictionary( \
        cv::aruco::PREDEFINED_DICTIONARY_NAME(dictionaryId));

    while (in_video.grab()) {
        cv::Mat image, image_copy;
        in_video.retrieve(image);
        image.copyTo(image_copy);
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::aruco::detectMarkers(image, dictionary, corners, ids);
        /*Estimate pose*/
        cv::aruco::estimatePoseSingleMarkers(corners, 0.05, intrinsic_matrix, distCoeffs, rvecs, tvecs);
        
        // If at least one marker detected
        if (ids.size() > 0)
            cv::aruco::drawDetectedMarkers(image_copy, corners, ids);

        for (int i = 0; i < rvecs.size(); ++i) {
                auto rvec = rvecs[i];
                auto tvec = tvecs[i];
                cv::aruco::drawAxis(image_copy, intrinsic_matrix, distCoeffs, rvec, tvec, 0.1);
            }
            
        imshow("Detected markers", image_copy);
        char key = (char)cv::waitKey(wait_time);
        if (key == 27)
            break;
    }
    in_video.release();
    return 0;
}