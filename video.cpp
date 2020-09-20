// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "net.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <vector>

#include "MJPEGWriter.h"

#define YOLOV4_TINY 1 //0 or undef for yolov4

struct Object
{
    cv::Rect_<float> rect;
    int label;
    float prob;
};

static ncnn::Net init_yolov4()
{
    ncnn::Net yolov4;

    yolov4.opt.use_vulkan_compute = true;

    // original pretrained model from https://github.com/AlexeyAB/darknet
    // the ncnn model https://drive.google.com/drive/folders/1YzILvh0SKQPS_lrb33dmGNq7aVTKPWS0?usp=sharing
    // the ncnn model https://github.com/nihui/ncnn-assets/tree/master/models
#if YOLOV4_TINY
    yolov4.load_param("yolov4-tiny-opt.param");
    yolov4.load_model("yolov4-tiny-opt.bin");
    const int target_size = 416;
#else
    yolov4.load_param("yolov4-opt.param");
    yolov4.load_model("yolov4-opt.bin");
    const int target_size = 608;
#endif

    return yolov4;
}

static int detect_yolov4(ncnn::Net& yolov4, const cv::Mat& bgr, std::vector<Object>& objects)
{
#if YOLOV4_TINY
    const int target_size = 416;
#else
    const int target_size = 608;
#endif
    ncnn::Extractor ex = yolov4.create_extractor();
    ex.set_num_threads(4);

    std::chrono::steady_clock::time_point Tbegin, Tend;

    int img_w = bgr.cols;
    int img_h = bgr.rows;

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(bgr.data, ncnn::Mat::PIXEL_BGR, bgr.cols, bgr.rows, target_size, target_size);

    const float mean_vals[3] = { 0, 0, 0 };
    const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
    in.substract_mean_normalize(mean_vals, norm_vals);

    ex.input("data", in);

    Tbegin = std::chrono::steady_clock::now();

    ncnn::Mat out;
    ex.extract("output", out);

    //     printf("%d %d %d\n", out.w, out.h, out.c);
    objects.clear();
    for (int i = 0; i < out.h; i++)
    {
        const float* values = out.row(i);

        Object object;
        object.label = values[0];
        object.prob = values[1];
        object.rect.x = values[2] * img_w;
        object.rect.y = values[3] * img_h;
        object.rect.width = values[4] * img_w - object.rect.x;
        object.rect.height = values[5] * img_h - object.rect.y;

        objects.push_back(object);
    }

    Tend = std::chrono::steady_clock::now();
    float f = std::chrono::duration_cast <std::chrono::milliseconds> (Tend - Tbegin).count();

    std::cout << "time : " << f / 1000.0 << " Sec" << std::endl;

    return 0;
}

static cv::Mat draw_objects(const cv::Mat& bgr, const std::vector<Object>& objects)
{
    static const char* class_names[] = { "background", "person", "bicycle",
                                        "car", "motorbike", "aeroplane", "bus", "train", "truck",
                                        "boat", "traffic light", "fire hydrant", "stop sign",
                                        "parking meter", "bench", "bird", "cat", "dog", "horse",
                                        "sheep", "cow", "elephant", "bear", "zebra", "giraffe",
                                        "backpack", "umbrella", "handbag", "tie", "suitcase",
                                        "frisbee", "skis", "snowboard", "sports ball", "kite",
                                        "baseball bat", "baseball glove", "skateboard", "surfboard",
                                        "tennis racket", "bottle", "wine glass", "cup", "fork",
                                        "knife", "spoon", "bowl", "banana", "apple", "sandwich",
                                        "orange", "broccoli", "carrot", "hot dog", "pizza", "donut",
                                        "cake", "chair", "sofa", "pottedplant", "bed", "diningtable",
                                        "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard",
                                        "cell phone", "microwave", "oven", "toaster", "sink",
                                        "refrigerator", "book", "clock", "vase", "scissors",
                                        "teddy bear", "hair drier", "toothbrush"
    };

    cv::Mat image = bgr.clone();

    for (size_t i = 0; i < objects.size(); i++)
    {
        const Object& obj = objects[i];

        fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
            obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

        cv::rectangle(image, obj.rect, cv::Scalar(255, 0, 0));

        char text[256];
        sprintf(text, "%s %.1f%%", class_names[obj.label], obj.prob * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        int x = obj.rect.x;
        int y = obj.rect.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > image.cols)
            x = image.cols - label_size.width;

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
            cv::Scalar(255, 255, 255), -1);

        cv::putText(image, text, cv::Point(x, y + label_size.height),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    }

    return image;
}

int main(int argc, char** argv)
{
    cv::VideoCapture capture(0);
    if (!capture.isOpened())
    {
        std::cout << "video not open." << std::endl;
        return 1;
    }
    //获取当前视频帧率
    double rate = capture.get(cv::CAP_PROP_FPS);
    //当前视频帧
    cv::Mat frame;
    //每一帧之间的延时
    //与视频的帧率相对应
    int delay = 1000 / rate;
    bool stop(false);

    ncnn::Net yolov4 = init_yolov4();

    MJPEGWriter server(7777);
    server.start();

    while (!stop)
    {
        if (!capture.read(frame))
        {
            std::cout << "no video frame" << std::endl;
            break;
        }

        //此处为添加对视频的每一帧的操作方法
        int frame_num = capture.get(cv::CAP_PROP_POS_FRAMES);
        std::cout << "Frame Num : " << frame_num << std::endl;
        if (frame_num == 20)
        {
            capture.set(cv::CAP_PROP_POS_FRAMES, 10);
        }

        std::vector<Object> objects;
        detect_yolov4(yolov4, frame, objects);

        cv::Mat result = draw_objects(frame, objects);
        server.write(result);
    }

    //关闭视频，手动调用析构函数（非必须）
    capture.release();
    return 0;
}
