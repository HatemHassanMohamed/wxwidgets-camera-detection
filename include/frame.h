#ifndef FRAME_H
#define FRAME_H

#include <wx/wx.h>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <memory>
#include <chrono>
#include <vector>

struct FrameLog {
    wxString timestamp;
    int frame_number;
    int person_count;
};

struct Detection {
    float x, y, width, height;
    float confidence;
};

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);
    ~MyFrame();

private:
    void OnButtonClick(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnStartCamera(wxCommandEvent& event);
    void OnStopCamera(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnExportLog(wxCommandEvent& event);
    void OnClearLog(wxCommandEvent& event);
    void UpdateFrame();
    wxBitmap MatToBitmap(const cv::Mat& mat);
    wxString GetCurrentTimestamp();
    void LogFrame(int frame_num, int person_count);
    void UpdateLogDisplay();
    void ExportLogToFile(const wxString& filename);
    std::vector<Detection> DetectObjects(const cv::Mat& frame);
    void InitializeYOLO();
    
    wxTextCtrl* m_textCtrl;
    wxTextCtrl* m_logCtrl;
    wxStaticBitmap* m_imageCtrl;
    wxButton* m_startBtn;
    wxButton* m_stopBtn;
    wxButton* m_clickBtn;
    wxButton* m_quitBtn;
    wxButton* m_exportBtn;
    wxButton* m_clearBtn;
    wxComboBox* m_cameraChoice;
    
    cv::VideoCapture m_cap;
    wxTimer m_timer;
    bool m_camera_running;
    int m_frame_count;
    int m_fps_counter;
    std::vector<FrameLog> m_frame_logs;
    std::chrono::high_resolution_clock::time_point m_start_time;
    std::chrono::high_resolution_clock::time_point m_fps_time;
    
    // YOLO detection with OpenCV DNN
    cv::dnn::Net m_net;
    bool m_yolo_initialized;
};

#endif // FRAME_H
