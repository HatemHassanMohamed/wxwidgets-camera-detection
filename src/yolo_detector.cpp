#include "yolo_detector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

YOLODetector::YOLODetector(const std::string& model_path)
    : initialized(false), input_width(640), input_height(640),
      confidence_threshold(0.45f), nms_threshold(0.45f) {
    
    try {
        // Create ONNX Runtime environment
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "YOLODetector");
        
        // Create session options
        Ort::SessionOptions session_options;
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        // Create session
        session = std::make_unique<Ort::Session>(*env, model_path.c_str(), session_options);
        
        std::cout << "YOLO ONNX model loaded successfully!" << std::endl;
        
        loadClassNames();
        initialized = true;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "Error loading YOLO model: " << e.what() << std::endl;
        initialized = false;
    } catch (const std::exception& e) {
        std::cerr << "Error loading YOLO model: " << e.what() << std::endl;
        initialized = false;
    }
}

YOLODetector::~YOLODetector() {
    session.reset();
    env.reset();
}

void YOLODetector::loadClassNames() {
    // COCO dataset class names
    class_names = {
        "person", "bicycle", "car", "motorbike", "aeroplane",
        "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "cat",
        "dog", "horse", "sheep", "cow", "elephant",
        "bear", "zebra", "giraffe", "backpack", "umbrella",
        "handbag", "tie", "suitcase", "frisbee", "skis",
        "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass",
        "cup", "fork", "knife", "spoon", "bowl",
        "banana", "apple", "sandwich", "orange", "broccoli",
        "carrot", "hot dog", "pizza", "donut", "cake",
        "chair", "sofa", "pottedplant", "bed", "diningtable",
        "toilet", "tvmonitor", "laptop", "mouse", "remote",
        "keyboard", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors",
        "teddy bear", "hair drier", "toothbrush"
    };
}

std::vector<Detection> YOLODetector::detect(const cv::Mat& frame) {
    std::vector<Detection> detections;
    
    if (!initialized || frame.empty()) {
        return detections;
    }
    
    try {
        // Prepare blob for network
        cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0, 
                                               cv::Size(input_width, input_height), 
                                               cv::Scalar(0, 0, 0), true, false);
        
        // Create input tensor
        std::vector<int64_t> input_shape = {1, 3, input_height, input_width};
        std::vector<float> input_data(blob.total());
        
        float* p_data = blob.ptr<float>();
        std::copy(p_data, p_data + blob.total(), input_data.begin());
        
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
        
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_data.data(), input_data.size(),
            input_shape.data(), input_shape.size());
        
        // Standard YOLO ONNX input/output names
        const char* input_names[] = {"images"};
        const char* output_names[] = {"output0"};
        
        // Run inference
        auto output_tensors = session->Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 1);
        
        // Parse output
        if (output_tensors.size() > 0) {
            auto& output = output_tensors.at(0);
            std::vector<int64_t> output_shape = output.GetTensorTypeAndShapeInfo().GetShape();
            float* output_data = output.GetTensorMutableData<float>();
            
            size_t output_size = 1;
            for (auto dim : output_shape) {
                output_size *= dim;
            }
            
            std::vector<float> output_vec(output_data, output_data + output_size);
            detections = parseOutput(output_vec, frame, output_shape);
        }
        
    } catch (const Ort::Exception& e) {
        std::cerr << "Error during detection: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during detection: " << e.what() << std::endl;
    }
    
    return detections;
}

std::vector<Detection> YOLODetector::parseOutput(const std::vector<float>& output, 
                                                 const cv::Mat& frame,
                                                 const std::vector<int64_t>& output_shape) {
    std::vector<Detection> detections;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> class_ids;
    
    float x_factor = frame.cols / (float)input_width;
    float y_factor = frame.rows / (float)input_height;
    
    // YOLOv8 output format: (1, 84, 8400) or (1, 8400, 84)
    // 84 = 4 (bbox) + 1 (objectness) + 79 (classes) for COCO
    
    int num_detections = output.size() / 84; // or output_shape[2] if (1, 84, 8400)
    
    for (int i = 0; i < num_detections; ++i) {
        float objectness = output[i * 84 + 4];
        
        if (objectness >= confidence_threshold) {
            float* class_scores = (float*)&output[i * 84 + 5];
            
            // Find max class score
            float max_score = 0.0f;
            int max_class = 0;
            
            for (int j = 0; j < 80; j++) { // COCO has 80 classes
                if (class_scores[j] > max_score) {
                    max_score = class_scores[j];
                    max_class = j;
                }
            }
            
            float confidence = objectness * max_score;
            
            if (confidence >= confidence_threshold) {
                // Only keep person detections (class 0)
                if (max_class == 0) {
                    float x = output[i * 84 + 0] * x_factor;
                    float y = output[i * 84 + 1] * y_factor;
                    float w = output[i * 84 + 2] * x_factor;
                    float h = output[i * 84 + 3] * y_factor;
                    
                    cv::Rect box(x - w/2, y - h/2, w, h);
                    
                    // Clamp to frame boundaries
                    box.x = std::max(0, box.x);
                    box.y = std::max(0, box.y);
                    box.width = std::min(box.width, frame.cols - box.x);
                    box.height = std::min(box.height, frame.rows - box.y);
                    
                    boxes.push_back(box);
                    confidences.push_back(confidence);
                    class_ids.push_back(max_class);
                }
            }
        }
    }
    
    // Apply NMS
    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, nms_threshold, nms_result);
    
    for (int i : nms_result) {
        Detection det;
        det.x = boxes[i].x;
        det.y = boxes[i].y;
        det.w = boxes[i].width;
        det.h = boxes[i].height;
        det.confidence = confidences[i];
        det.class_id = class_ids[i];
        detections.push_back(det);
    }
    
    return detections;
}

cv::Mat YOLODetector::drawDetections(const cv::Mat& frame, const std::vector<Detection>& detections) {
    cv::Mat result = frame.clone();
    
    for (const auto& det : detections) {
        // Draw bounding box
        cv::rectangle(result, 
                     cv::Rect(det.x, det.y, det.w, det.h), 
                     cv::Scalar(0, 255, 0), 2);
        
        // Draw label with confidence
        std::string label = "Person: " + std::to_string((int)(det.confidence * 100)) + "%";
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        
        cv::rectangle(result, 
                     cv::Point(det.x, det.y - text_size.height - 5),
                     cv::Point(det.x + text_size.width, det.y),
                     cv::Scalar(0, 255, 0), -1);
        
        cv::putText(result, label, 
                   cv::Point(det.x, det.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                   cv::Scalar(0, 0, 0), 1);
    }
    
    return result;
}
