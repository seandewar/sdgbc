#ifndef SDGBC_MAIN_FRAME_H_
#define SDGBC_MAIN_FRAME_H_

#include "audio/sfml_apu_out.h"
#include "wxui/lcd_canvas.h"
#include "wxui/wx.h"
#include "emulator.h"
#include <wx/dnd.h>

class MainFrame : public wxFrame {
public:
  MainFrame();

  bool LoadCartridgeRomFile(std::string filePath = {});

  bool ExportCartridgeBatteryExtRam(std::string filePath = {});
  bool ImportCartridgeBatteryExtRam(std::string filePath = {});

private:
  wxDECLARE_EVENT_TABLE();

  LcdCanvas* lcdCanvas_;
  SfmlApuSoundStream audioOut_;
  Emulator emulator_;

  void RecursivelyConnectKeyEvents(wxWindow* childComponent);

  void UpdateUIState();
  void UpdateTitle();
  void UpdateMenu();

  bool HandleJoypadKeyEvent(wxKeyEvent& event);

  void OnClose(wxCloseEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnExit(wxCommandEvent& event);

  void OnKeyDown(wxKeyEvent& event);
  void OnKeyUp(wxKeyEvent& event);

  void OnOpenRomFile(wxCommandEvent& event);
  void OnImportBattery(wxCommandEvent& event);
  void OnExportBattery(wxCommandEvent& event);

  void OnReset(wxCommandEvent& event);
  void OnResetInDmgMode(wxCommandEvent& event);
  void OnPause(wxCommandEvent& event);
  void OnLimitFramerate(wxCommandEvent& event);

  void OnEnableBg(wxCommandEvent& event);
  void OnEnableBgWindow(wxCommandEvent& event);
  void OnEnableSprites(wxCommandEvent& event);
  void OnLimitScanlineSprites(wxCommandEvent& event);
  void OnMaintainAspectRatio(wxCommandEvent& event);
  void OnSmoothenVideo(wxCommandEvent& event);

  void OnMuteSound(wxCommandEvent& event);
  void OnMuteCh1(wxCommandEvent& event);
  void OnMuteCh2(wxCommandEvent& event);
  void OnMuteCh3(wxCommandEvent& event);
  void OnMuteCh4(wxCommandEvent& event);

  void OnJoypadImpossibleInputs(wxCommandEvent& event);

  void OnJoypadControls(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
};

class MainFrameFileDropTarget : public wxFileDropTarget {
public:
  explicit MainFrameFileDropTarget(MainFrame& mainFrame);

  bool OnDropFiles(wxCoord x, wxCoord y,
                   const wxArrayString& filenames) override;

private:
  MainFrame& mainFrame_;
};

#endif // SDGBC_MAIN_FRAME_H_
