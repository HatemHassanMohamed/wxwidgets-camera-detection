#include <wx/wx.h>
#include "frame.h"

class MyApp : public wxApp {
public:
    bool OnInit() override;
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    MyFrame* frame = new MyFrame("wxWidgets Application");
    frame->Show(true);
    return true;
}
