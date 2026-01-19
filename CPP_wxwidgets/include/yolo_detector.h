#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <memory>

struct Detection {
    float x, y, w, h;  // bounding box
    float confidence;
    int class_id;
};

class YOLODetector {
public:
    YOLODetector(const std::string& model_path);
    ~YOLODetector();
    
    std::vector<Detection> detect(const cv::Mat& frame);
    cv::Mat drawDetections(const cv::Mat& frame, const std::vector<Detection>& detections);
    bool isInitialized() const { return initialized; }
    
private:
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::Env> env;
    bool initialized;
    std::vector<std::string> class_names;
    int input_width;
    int input_height;
    float confidence_threshold;
    float nms_threshold;
    
    std::vector<Detection> parseOutput(const std::vector<float>& output, 
                                       const cv::Mat& frame, 
                                       const std::vector<int64_t>& output_shape);
    void loadClassNames();
};

#endif // YOLO_DETECTOR_H
