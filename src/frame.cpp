#include "frame.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 700)),
      m_timer(this, wxID_ANY), m_camera_running(false), m_frame_count(0), m_fps_counter(0),
      m_yolo_initialized(false) {
    
    // Initialize YOLO
    InitializeYOLO();
    // Create main panel
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // ==================== LEFT SIDE - VIDEO DISPLAY ====================
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    // Camera selection
    wxBoxSizer* cameraSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* cameraLabel = new wxStaticText(panel, wxID_ANY, "Camera:");
    wxString cameras[] = {"Camera 0", "Camera 1", "Camera 2"};
    m_cameraChoice = new wxComboBox(panel, wxID_ANY, "Camera 0", wxDefaultPosition, 
                                    wxDefaultSize, 3, cameras, wxCB_READONLY);
    cameraSizer->Add(cameraLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    cameraSizer->Add(m_cameraChoice, 1, wxALL | wxEXPAND, 5);
    leftSizer->Add(cameraSizer, 0, wxALL | wxEXPAND, 5);
    
    // Video display
    wxStaticBoxSizer* videoBorder = new wxStaticBoxSizer(wxVERTICAL, panel, "Live Camera Feed");
    wxBitmap placeholder(640, 480);
    m_imageCtrl = new wxStaticBitmap(panel, wxID_ANY, placeholder);
    m_imageCtrl->SetMinSize(wxSize(640, 480));
    videoBorder->Add(m_imageCtrl, 1, wxALL | wxEXPAND, 5);
    leftSizer->Add(videoBorder, 1, wxALL | wxEXPAND, 10);
    
    // ==================== RIGHT SIDE - CONTROLS & INFO ====================
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* titleLabel = new wxStaticText(panel, wxID_ANY, "Real-Time Camera Viewer");
    wxFont titleFont = titleLabel->GetFont();
    titleFont.MakeBold();
    titleFont.MakeLarger();
    titleLabel->SetFont(titleFont);
    rightSizer->Add(titleLabel, 0, wxALL, 10);
    
    // Camera control buttons
    wxBoxSizer* btnSizer1 = new wxBoxSizer(wxHORIZONTAL);
    m_startBtn = new wxButton(panel, wxID_ANY, "Start Camera");
    m_stopBtn = new wxButton(panel, wxID_ANY, "Stop Camera");
    m_stopBtn->Disable();
    btnSizer1->Add(m_startBtn, 1, wxALL, 5);
    btnSizer1->Add(m_stopBtn, 1, wxALL, 5);
    rightSizer->Add(btnSizer1, 0, wxALL | wxEXPAND, 5);
    
    // Information box
    wxStaticBoxSizer* infoBorder = new wxStaticBoxSizer(wxVERTICAL, panel, "Status");
    wxString infoText = "Status: Ready\nFrames: 0\nFPS: 0\nUptime: 00:00:00";
    m_textCtrl = new wxTextCtrl(panel, wxID_ANY, infoText, wxDefaultPosition, wxSize(400, 100),
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    infoBorder->Add(m_textCtrl, 0, wxALL | wxEXPAND, 5);
    rightSizer->Add(infoBorder, 0, wxALL | wxEXPAND, 5);
    
    // Frame Log
    wxStaticBoxSizer* logBorder = new wxStaticBoxSizer(wxVERTICAL, panel, "Frame History (Last 20)");
    m_logCtrl = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(400, 200),
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    logBorder->Add(m_logCtrl, 1, wxALL | wxEXPAND, 5);
    rightSizer->Add(logBorder, 1, wxALL | wxEXPAND, 5);
    
    // Export/Clear buttons
    wxBoxSizer* btnSizer2 = new wxBoxSizer(wxHORIZONTAL);
    m_exportBtn = new wxButton(panel, wxID_ANY, "Export Log");
    m_clearBtn = new wxButton(panel, wxID_ANY, "Clear Log");
    btnSizer2->Add(m_exportBtn, 1, wxALL, 5);
    btnSizer2->Add(m_clearBtn, 1, wxALL, 5);
    rightSizer->Add(btnSizer2, 0, wxALL | wxEXPAND, 5);
    
    // Other buttons
    wxBoxSizer* btnSizer3 = new wxBoxSizer(wxVERTICAL);
    m_clickBtn = new wxButton(panel, wxID_ANY, "Test Button");
    m_quitBtn = new wxButton(panel, wxID_EXIT, "Quit");
    btnSizer3->Add(m_clickBtn, 0, wxALL | wxEXPAND, 5);
    btnSizer3->Add(m_quitBtn, 0, wxALL | wxEXPAND, 5);
    rightSizer->Add(btnSizer3, 0, wxALL | wxEXPAND, 5);
    
    // Add sizers to main sizer
    mainSizer->Add(leftSizer, 2, wxALL | wxEXPAND, 5);
    mainSizer->Add(rightSizer, 1, wxALL | wxEXPAND, 5);
    
    panel->SetSizer(mainSizer);
    
    // Bind events
    Bind(wxEVT_BUTTON, &MyFrame::OnStartCamera, this, m_startBtn->GetId());
    Bind(wxEVT_BUTTON, &MyFrame::OnStopCamera, this, m_stopBtn->GetId());
    Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this, m_clickBtn->GetId());
    Bind(wxEVT_BUTTON, &MyFrame::OnExportLog, this, m_exportBtn->GetId());
    Bind(wxEVT_BUTTON, &MyFrame::OnClearLog, this, m_clearBtn->GetId());
    Bind(wxEVT_BUTTON, &MyFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_TIMER, &MyFrame::OnTimer, this);
    
    std::cout << "Camera application initialized - ready to stream" << std::endl;
}

MyFrame::~MyFrame() {
    if (m_camera_running) {
        m_timer.Stop();
        if (m_cap.isOpened()) {
            m_cap.release();
        }
    }
}

wxString MyFrame::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return wxString(ss.str());
}

void MyFrame::LogFrame(int frame_num, int person_count) {
    FrameLog log;
    log.timestamp = GetCurrentTimestamp();
    log.frame_number = frame_num;
    log.person_count = person_count;
    m_frame_logs.push_back(log);
    UpdateLogDisplay();
}

void MyFrame::UpdateLogDisplay() {
    wxString logText;
    
    // Show last 20 frames
    int start = std::max(0, (int)m_frame_logs.size() - 20);
    
    for (int i = start; i < (int)m_frame_logs.size(); ++i) {
        const auto& log = m_frame_logs[i];
        wxString line = wxString::Format("[%s] Frame: %d | Persons: %d\n",
                                        log.timestamp,
                                        log.frame_number,
                                        log.person_count);
        logText += line;
    }
    
    m_logCtrl->SetValue(logText);
    m_logCtrl->SetInsertionPointEnd();
}

void MyFrame::OnStartCamera(wxCommandEvent& event) {
    // Get selected camera
    int camera_idx = m_cameraChoice->GetSelection();
    
    // Open camera
    m_cap.open(camera_idx);
    
    if (!m_cap.isOpened()) {
        wxMessageBox("Failed to open camera " + std::to_string(camera_idx) + "!\n"
                    "Make sure your camera is connected and not in use.",
                    "Camera Error", wxOK | wxICON_ERROR);
        return;
    }
    
    // Set camera properties
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    m_cap.set(cv::CAP_PROP_FPS, 30);
    m_cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    m_camera_running = true;
    m_frame_count = 0;
    m_fps_counter = 0;
    m_start_time = std::chrono::high_resolution_clock::now();
    m_fps_time = std::chrono::high_resolution_clock::now();
    m_frame_logs.clear();
    
    m_startBtn->Disable();
    m_stopBtn->Enable();
    m_cameraChoice->Disable();
    
    m_logCtrl->SetValue("Camera connected. Streaming...\n");
    
    // Start timer for video capture (30 FPS = 33ms)
    m_timer.Start(33, wxTIMER_CONTINUOUS);
    
    std::cout << "Camera started: " << camera_idx << std::endl;
}

void MyFrame::OnStopCamera(wxCommandEvent& event) {
    m_timer.Stop();
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    
    m_camera_running = false;
    m_startBtn->Enable();
    m_stopBtn->Disable();
    m_cameraChoice->Enable();
    
    wxString statusText = wxString::Format(
        "Status: Stopped\nTotal Frames: %d\nFrames Logged: %lu",
        m_frame_count,
        m_frame_logs.size());
    
    m_textCtrl->SetValue(statusText);
}

void MyFrame::OnButtonClick(wxCommandEvent& event) {
    m_logCtrl->AppendText("[Test] Button clicked at " + GetCurrentTimestamp() + "\n");
}

void MyFrame::OnExportLog(wxCommandEvent& event) {
    if (m_frame_logs.empty()) {
        wxMessageBox("No frame logs to export!", "Info", wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxFileDialog saveFileDialog(this, "Save frame log", "", "",
                               "CSV files (*.csv)|*.csv|Text files (*.txt)|*.txt",
                               wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveFileDialog.ShowModal() == wxID_OK) {
        wxString path = saveFileDialog.GetPath();
        ExportLogToFile(path);
        wxMessageBox("Log exported successfully to:\n" + path, "Export Complete", wxOK | wxICON_INFORMATION);
    }
}

void MyFrame::OnClearLog(wxCommandEvent& event) {
    if (!m_frame_logs.empty()) {
        wxMessageDialog dlg(this, "Clear all frame logs?", "Confirm",
                           wxYES_NO | wxICON_QUESTION);
        if (dlg.ShowModal() == wxID_YES) {
            m_frame_logs.clear();
            m_logCtrl->SetValue("");
        }
    }
}

void MyFrame::ExportLogToFile(const wxString& filename) {
    std::ofstream file(filename.ToStdString());
    
    if (!file.is_open()) {
        wxMessageBox("Failed to create file!", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    // Write CSV header
    file << "Timestamp,Frame_Number,Person_Count\n";
    
    // Write data
    for (const auto& log : m_frame_logs) {
        file << log.timestamp.ToStdString() << ","
             << log.frame_number << ","
             << log.person_count << "\n";
    }
    
    file.close();
}

void MyFrame::OnQuit(wxCommandEvent& event) {
    if (m_camera_running) {
        m_timer.Stop();
        if (m_cap.isOpened()) {
            m_cap.release();
        }
    }
    Close(true);
}

void MyFrame::OnTimer(wxTimerEvent& event) {
    UpdateFrame();
}

void MyFrame::InitializeYOLO() {
    try {
        // Try to load YOLOv8s ONNX model using OpenCV DNN
        std::vector<std::string> model_paths = {
            "/home/hatem/CPP_wxwidgets/build/yolov8s.onnx",
            "./yolov8s.onnx",
            "yolov8s.onnx"
        };
        
        for (const auto& model_path : model_paths) {
            std::cout << "Trying to load YOLO model from: " << model_path << std::endl;
            
            // Check if file exists
            std::ifstream f(model_path);
            if (!f.good()) {
                std::cerr << "File not found: " << model_path << std::endl;
                continue;
            }
            f.close();
            
            try {
                m_net = cv::dnn::readNetFromONNX(model_path);
                if (m_net.empty()) {
                    std::cerr << "Failed to load network from " << model_path << std::endl;
                    continue;
                }
                
                m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
                m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
                
                m_yolo_initialized = true;
                std::cout << "✓ YOLO model loaded successfully from: " << model_path << std::endl;
                return;
            } catch (const cv::Exception& e) {
                std::cerr << "OpenCV error loading " << model_path << ": " << e.what() << std::endl;
            }
        }
        
        std::cerr << "Warning: YOLO model not loaded. Running detection-free mode." << std::endl;
        m_yolo_initialized = false;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize YOLO: " << e.what() << std::endl;
        m_yolo_initialized = false;
    }
}

std::vector<Detection> MyFrame::DetectObjects(const cv::Mat& frame) {
    std::vector<Detection> detections;
    
    if (!m_yolo_initialized || m_net.empty()) {
        return detections;
    }
    
    try {
        // Prepare input blob
        cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0, cv::Size(640, 640),
                                             cv::Scalar(0, 0, 0), true, false);
        
        // Set input
        m_net.setInput(blob);
        
        // Get output layer names
        std::vector<cv::String> outNames = m_net.getUnconnectedOutLayersNames();
        std::vector<cv::Mat> outs;
        
        // Forward pass
        m_net.forward(outs, outNames);
        
        // Process detections
        for (size_t i = 0; i < outs.size(); i++) {
            float* data = (float*)outs[i].data;
            int rows = outs[i].rows;
            
            for (int j = 0; j < rows; j++) {
                // YOLOv8 output format: [cx, cy, w, h, conf_person, ...]
                float conf = data[j * outs[i].cols + 4];
                
                if (conf > 0.5f) {  // Confidence threshold
                    float cx = data[j * outs[i].cols + 0];
                    float cy = data[j * outs[i].cols + 1];
                    float w = data[j * outs[i].cols + 2];
                    float h = data[j * outs[i].cols + 3];
                    
                    Detection det;
                    det.x = cx;
                    det.y = cy;
                    det.width = w;
                    det.height = h;
                    det.confidence = conf;
                    detections.push_back(det);
                }
            }
        }
    } catch (const cv::Exception& e) {
        std::cerr << "Detection error: " << e.what() << std::endl;
    }
    
    return detections;
}

void MyFrame::UpdateFrame() {
    cv::Mat frame;
    
    if (!m_cap.read(frame)) {
        wxMessageBox("Failed to read frame from camera!", "Camera Error", wxOK | wxICON_ERROR);
        // Stop camera when frame read fails
        m_timer.Stop();
        if (m_cap.isOpened()) {
            m_cap.release();
        }
        m_camera_running = false;
        m_startBtn->Enable();
        m_stopBtn->Disable();
        m_cameraChoice->Enable();
        return;
    }
    
    // Resize frame to fit display
    cv::Mat displayFrame;
    cv::resize(frame, displayFrame, cv::Size(640, 480));
    
    // Increment frame counter
    m_frame_count++;
    m_fps_counter++;
    
    // Run object detection
    std::vector<Detection> detections = DetectObjects(displayFrame);
    int person_count = 0;
    
    // Draw detections
    for (const auto& det : detections) {
        if (det.confidence > 0.5f) {
            person_count++;
            int x = (int)(det.x - det.width / 2);
            int y = (int)(det.y - det.height / 2);
            int w = (int)det.width;
            int h = (int)det.height;
            
            cv::rectangle(displayFrame, cv::Point(x, y), cv::Point(x + w, y + h),
                         cv::Scalar(0, 255, 0), 2);
            cv::putText(displayFrame, "Person: " + std::to_string((int)(det.confidence * 100)) + "%",
                       cv::Point(x, y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 255, 0), 1);
        }
    }
    
    // Log every 10th frame
    if (m_frame_count % 10 == 0) {
        LogFrame(m_frame_count, person_count);
    }
    
    // Add timestamp and info to frame
    wxString timestamp = GetCurrentTimestamp();
    cv::putText(displayFrame, "Camera Feed - " + timestamp.ToStdString(),
               cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6,
               cv::Scalar(0, 255, 0), 2);
    
    cv::putText(displayFrame, "Frame: " + std::to_string(m_frame_count) + " | Persons: " + std::to_string(person_count),
               cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5,
               cv::Scalar(0, 255, 0), 1);
    
    // Calculate FPS
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - m_fps_time).count();
    
    float fps = 0;
    if (elapsed >= 1000) {
        fps = (m_fps_counter * 1000.0f) / elapsed;
        m_fps_counter = 0;
        m_fps_time = current_time;
    }
    
    cv::putText(displayFrame, "FPS: " + std::to_string((int)fps),
               cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.5,
               cv::Scalar(0, 255, 0), 1);
    
    // Convert to wxBitmap and display
    wxBitmap bitmap = MatToBitmap(displayFrame);
    m_imageCtrl->SetBitmap(bitmap);
    
    // Calculate uptime
    auto uptime_duration = std::chrono::duration_cast<std::chrono::seconds>(
        current_time - m_start_time);
    int hours = uptime_duration.count() / 3600;
    int minutes = (uptime_duration.count() % 3600) / 60;
    int seconds = uptime_duration.count() % 60;
    
    // Update info text
    wxString infoText = wxString::Format(
        "Status: RUNNING\nFrames: %d\nPersons: %d\nFPS: %.1f\nUptime: %02d:%02d:%02d",
        m_frame_count,
        person_count,
        fps,
        hours, minutes, seconds);
    
    m_textCtrl->SetValue(infoText);
}

wxBitmap MyFrame::MatToBitmap(const cv::Mat& mat) {
    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    } else {
        rgbMat = mat;
    }
    
    wxImage image(rgbMat.cols, rgbMat.rows, rgbMat.data, true);
    return wxBitmap(image);
}
