#include "debug/cpu_cmd_mode.h"
#include "wxui/app.h"
#include "wxui/main_frame.h"
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#endif

wxIMPLEMENT_APP(App);

bool App::OnInit() {
  // allow wxWidgets to initialize the wxApp first
  if (!wxApp::OnInit()) {
    return false;
  }

  if (cpuCmdMode_) {
    // CPU command-line debugger mode
    if (!InitCpuCmdMode()) {
      return false;
    }
  } else {
    // normal GUI mode
    auto mainFrame = new MainFrame;
    mainFrame->Show(true);
    SetTopWindow(mainFrame);

    // load our startup ROM file if specified
    if (!startupRomFilePath_.empty()) {
      mainFrame->LoadCartridgeRomFile(startupRomFilePath_);
    }
  }

  return true;
}

int App::OnRun() {
  if (cpuCmdMode_) {
    return CpuCmdMode().Run(startupRomFilePath_);
  } else {
    return wxApp::OnRun();
  }
}

#ifdef _WIN32
bool App::InitWin32Console() {
  // implementation derived from Roger Sanders' @
  // https://stackoverflow.com/a/25927081

  // NOTE: attaching to an existing console causes issues with programs like
  // cmd.exe that continue to capture from stdin, so we'll always just alloc
  // our own console instead :)
  if (AllocConsole()) {
    // attach console output to stdout/err and console input to stdin streams
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    // clear stream error states
    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
    std::wcout.clear();
    std::wcerr.clear();
    std::wcin.clear();
    return true;
  }

  return false;
}
#endif

bool App::InitCpuCmdMode() {
  // redirect wx logs to stderr
  delete wxLog::SetActiveTarget(new wxLogStream(&std::cerr));

#ifdef _WIN32
  // win32 won't handle console IO for us - set it up manually
  if (!InitWin32Console()) {
    return false;
  }
#endif

  return true;
}

bool App::IsInCpuCmdMode() const {
  return cpuCmdMode_;
}

void App::OnInitCmdLine(wxCmdLineParser& parser) {
  parser.SetSwitchChars("-");
  parser.AddParam("path of the program ROM file to be loaded at startup",
                  wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
  parser.AddLongSwitch("cpu-cmd", "start using the command-line CPU debugger");
}

bool App::OnCmdLineParsed(wxCmdLineParser& parser) {
  // read startup ROM file paths, if any
  if (parser.GetParamCount() > 0) {
    startupRomFilePath_ = parser.GetParam(0);
  }

  cpuCmdMode_ = parser.Found("cpu-cmd");
  return true;
}

std::string App::GetStartupRomFilePath() const {
  return startupRomFilePath_;
}
