#ifndef SDGBC_APP_H_
#define SDGBC_APP_H_

#include "wxui/wx.h"
#include <wx/cmdline.h>

class App : public wxApp {
public:
  bool OnInit() override;
  int OnRun() override;

  void OnInitCmdLine(wxCmdLineParser& parser) override;
  bool OnCmdLineParsed(wxCmdLineParser& parser) override;

  bool InitCpuCmdMode();
#ifdef _WIN32
  bool InitWin32Console();
#endif

  bool IsInCpuCmdMode() const;
  std::string GetStartupRomFilePath() const;

private:
  bool cpuCmdMode_;
  std::string startupRomFilePath_;
};

#endif // SDGBC_APP_H_
