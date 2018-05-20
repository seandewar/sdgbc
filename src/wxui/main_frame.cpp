#include "wxui/about_dialog.h"
#include "wxui/joypad_dialog.h"
#include "wxui/main_frame.h"
#include "wxui/xpm/xpm.h"
#include <wx/filename.h>

enum MenuItemId {
  kMenuIdOpenRomFile = wxID_HIGHEST + 1,
  kMenuIdImportBattery,
  kMenuIdExportBattery,

  kMenuIdReset,
  kMenuIdResetInDmgMode,
  kMenuIdPause,
  kMenuIdLimitFramerate,

  kMenuIdEnableBg,
  kMenuIdEnableBgWindow,
  kMenuIdEnableSprites,
  kMenuIdLimitScanlineSprites,
  kMenuIdMaintainAspectRatio,
  kMenuIdSmoothenVideo,

  kMenuIdMuteSound,
  kMenuIdMuteCh1,
  kMenuIdMuteCh2,
  kMenuIdMuteCh3,
  kMenuIdMuteCh4,

  kMenuIdJoypadImpossibleInputs,
  kMenuIdJoypadControls
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_CLOSE(MainFrame::OnClose)
  EVT_SIZE(MainFrame::OnSize)
  EVT_MENU(wxID_EXIT, MainFrame::OnExit)

  EVT_KEY_DOWN(MainFrame::OnKeyDown)
  EVT_KEY_UP(MainFrame::OnKeyUp)

  EVT_MENU(kMenuIdOpenRomFile, MainFrame::OnOpenRomFile)
  EVT_MENU(kMenuIdImportBattery, MainFrame::OnImportBattery)
  EVT_MENU(kMenuIdExportBattery, MainFrame::OnExportBattery)

  EVT_MENU(kMenuIdReset, MainFrame::OnReset)
  EVT_MENU(kMenuIdResetInDmgMode, MainFrame::OnResetInDmgMode)
  EVT_MENU(kMenuIdPause, MainFrame::OnPause)
  EVT_MENU(kMenuIdLimitFramerate, MainFrame::OnLimitFramerate)

  EVT_MENU(kMenuIdEnableBg, MainFrame::OnEnableBg)
  EVT_MENU(kMenuIdEnableBgWindow, MainFrame::OnEnableBgWindow)
  EVT_MENU(kMenuIdEnableSprites, MainFrame::OnEnableSprites)
  EVT_MENU(kMenuIdLimitScanlineSprites, MainFrame::OnLimitScanlineSprites)
  EVT_MENU(kMenuIdMaintainAspectRatio, MainFrame::OnMaintainAspectRatio)
  EVT_MENU(kMenuIdSmoothenVideo, MainFrame::OnSmoothenVideo)

  EVT_MENU(kMenuIdMuteSound, MainFrame::OnMuteSound)
  EVT_MENU(kMenuIdMuteCh1, MainFrame::OnMuteCh1)
  EVT_MENU(kMenuIdMuteCh2, MainFrame::OnMuteCh2)
  EVT_MENU(kMenuIdMuteCh3, MainFrame::OnMuteCh3)
  EVT_MENU(kMenuIdMuteCh4, MainFrame::OnMuteCh4)

  EVT_MENU(kMenuIdJoypadImpossibleInputs, MainFrame::OnJoypadImpossibleInputs)

  EVT_MENU(kMenuIdJoypadControls, MainFrame::OnJoypadControls)
  EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "sdgbc"), lcdCanvas_(nullptr) {
  // create a bundle of all our different sized icons to use as the window icon
  wxIconBundle iconBundle;
  iconBundle.AddIcon(wxIcon(xpmSdgbc16));
  iconBundle.AddIcon(wxIcon(xpmSdgbc24));
  iconBundle.AddIcon(wxIcon(xpmSdgbc32));
  iconBundle.AddIcon(wxIcon(xpmSdgbc48));
  iconBundle.AddIcon(wxIcon(xpmSdgbc64));
  iconBundle.AddIcon(wxIcon(xpmSdgbc96));
  iconBundle.AddIcon(wxIcon(xpmSdgbc256));
  SetIcons(iconBundle);

  // create menu items
  auto menuFile = new wxMenu;
  menuFile->Append(kMenuIdOpenRomFile, "&Open ROM File...\tCtrl+O");
  menuFile->AppendSeparator();
  menuFile->Append(kMenuIdImportBattery,
                   "&Import Battery-Packed RAM Snapshot...");
  menuFile->Append(kMenuIdExportBattery,
                   "&Export Battery-Packed RAM Snapshot...");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  auto menuEmulator = new wxMenu;
  menuEmulator->Append(kMenuIdReset, "&Reset\tCtrl+R");

  menuEmulator->AppendCheckItem(kMenuIdPause, "&Pause\tCtrl+P");
  menuEmulator->Check(kMenuIdPause, emulator_.IsPaused());

  menuEmulator->AppendSeparator();
  menuEmulator->Append(kMenuIdResetInDmgMode,
                       "Reset in Game Boy Compatibility Mode");

  menuEmulator->AppendSeparator();
  menuEmulator->AppendCheckItem(kMenuIdLimitFramerate,
                                "&Limit Emulation Speed\tCtrl+L");

  auto menuVideo = new wxMenu;
  menuVideo->AppendCheckItem(kMenuIdEnableBg, "Render Background Layer");
  menuVideo->AppendCheckItem(kMenuIdEnableBgWindow, "Render Window Layer");
  menuVideo->AppendCheckItem(kMenuIdEnableSprites,
                             "Render Sprite (Object) Layer");

  menuVideo->AppendSeparator();
  menuVideo->AppendCheckItem(kMenuIdLimitScanlineSprites,
                             "Emulate Scanline Sprite Limit");

  menuVideo->AppendSeparator();
  menuVideo->AppendCheckItem(kMenuIdMaintainAspectRatio,
                             "&Maintain Aspect Ratio");
  menuVideo->AppendCheckItem(kMenuIdSmoothenVideo, "&Smoothen Output");

  auto menuAudio = new wxMenu;
  menuAudio->AppendCheckItem(kMenuIdMuteSound, "&Mute Audio\tCtrl+M");

  menuAudio->AppendSeparator();
  menuAudio->AppendCheckItem(kMenuIdMuteCh1,
                             "Mute Sound Channel 1 (Square with Sweep)");
  menuAudio->AppendCheckItem(kMenuIdMuteCh2,
                             "Mute Sound Channel 2 (Square)");
  menuAudio->AppendCheckItem(kMenuIdMuteCh3,
                             "Mute Sound Channel 3 (Wave)");
  menuAudio->AppendCheckItem(kMenuIdMuteCh4,
                             "Mute Sound Channel 4 (Noise)");

  auto menuJoypad = new wxMenu;
  menuJoypad->AppendCheckItem(kMenuIdJoypadImpossibleInputs,
                              "&Allow Impossible Inputs\tCtrl+I");

  auto menuHelp = new wxMenu;
  menuHelp->Append(kMenuIdJoypadControls, "&Joypad Controls");
  menuHelp->Append(wxID_ABOUT);

  // add menu items to menu bar
  auto menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuEmulator, "&Emulation");
  menuBar->Append(menuVideo, "&Video");
  menuBar->Append(menuAudio, "&Audio");
  menuBar->Append(menuJoypad, "&Joypad");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  // add main screen elements
  auto mainSizer = new wxBoxSizer(wxVERTICAL);

  // add the lcd video canvas
  lcdCanvas_ = new LcdCanvas(this, wxID_ANY, wxPoint(0, 0),
                             wxSize(kLcdWidthPixels, kLcdHeightPixels));
  emulator_.SetVideoLcd(lcdCanvas_);
  mainSizer->Add(lcdCanvas_, 1, wxEXPAND | wxALL, 0);

  // manually connect key events so we can capture them
  RecursivelyConnectKeyEvents(this);

  // allow ROM loads via file drops onto the window
  SetDropTarget(new MainFrameFileDropTarget(*this));

  // position controls inside sizer correctly and fit the window to accommodate
  mainSizer->Layout();
  SetSizerAndFit(mainSizer);
  SetSize(wxSize(600, 570)); // initial size of the window

  emulator_.SetApuOutput(&audioOut_);
  audioOut_.StartStreaming();

  UpdateUIState();
}

void MainFrame::UpdateUIState() {
  UpdateTitle();
  UpdateMenu();
}

void MainFrame::UpdateTitle() {
  wxString title;
  if (emulator_.IsCartridgeRomLoaded()) {
    title << emulator_.GetCartridgeRomFileName()
          << (emulator_.IsPaused() ? " (Paused)" : "")
          << (emulator_.GetCartridgeExtMeta().hasBattery ? " [Battery]" : "")
          << (emulator_.IsInCgbMode() ? "" : " [Game Boy compatibility mode]")
          << " - ";
  }

  title << "sdgbc";
  SetTitle(title);
}

void MainFrame::UpdateMenu() {
  const bool romLoaded = emulator_.IsCartridgeRomLoaded(),
             romBattery = emulator_.GetCartridgeExtMeta().hasBattery &&
                          romLoaded;

  auto menu = GetMenuBar();

  // file menu
  menu->Enable(kMenuIdImportBattery, romBattery);
  menu->Enable(kMenuIdExportBattery, romBattery);

  // emulation menu
  menu->Enable(kMenuIdReset, romLoaded);
  menu->Enable(kMenuIdResetInDmgMode, romLoaded);
  menu->Check(kMenuIdLimitFramerate, emulator_.IsLimitingFramerate());

  // video menu
  menu->Check(kMenuIdEnableBg, emulator_.IsVideoBgRenderEnabled());
  menu->Check(kMenuIdEnableBgWindow, emulator_.IsVideoBgWindowRenderEnabled());
  menu->Check(kMenuIdEnableSprites, emulator_.IsVideoSpritesRenderEnabled());
  menu->Check(kMenuIdLimitScanlineSprites,
              emulator_.IsVideoScanlineSpritesLimiterEnabled());
  menu->Check(kMenuIdMaintainAspectRatio,
              lcdCanvas_->IsMaintainingAspectRatio());
  menu->Check(kMenuIdSmoothenVideo, lcdCanvas_->IsSmoothFilterEnabled());

  // audio menu
  menu->Check(kMenuIdMuteSound, audioOut_.AudioIsMuted());
  menu->Check(kMenuIdMuteCh1, emulator_.IsApuCh1Muted());
  menu->Check(kMenuIdMuteCh2, emulator_.IsApuCh2Muted());
  menu->Check(kMenuIdMuteCh3, emulator_.IsApuCh3Muted());
  menu->Check(kMenuIdMuteCh4, emulator_.IsApuCh4Muted());

  // joypad menu
  menu->Check(kMenuIdJoypadImpossibleInputs,
              emulator_.AreJoypadImpossibleInputsAllowed());
}

bool MainFrame::LoadCartridgeRomFile(std::string filePath) {
  // prompt the user for a file if empty filePath given
  if (filePath.empty()) {
    wxFileDialog openFileDialog(this, "Open ROM File", wxEmptyString,
                                wxEmptyString,
                                "Game Boy and Game Boy Color ROMs (*.gb;*.gbc)|"
                                "*.gb;*.gbc|"
                                "All Files (*.*)|*.*",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
      return false;
    }

    filePath = openFileDialog.GetPath().ToStdString();
  }

  const auto fileName = wxFileName(filePath).GetFullName().ToStdString();
  const auto result = emulator_.LoadCartridgeRomFile(filePath, fileName);
  if (result != RomLoadResult::Ok) {
    wxMessageDialog(this, Cartridge::GetRomLoadResultAsMessage(result),
                    "Failed to load ROM file", wxICON_ERROR | wxOK | wxCENTER)
        .ShowModal();
  }

  UpdateUIState();
  return result == RomLoadResult::Ok;
}

void MainFrame::OnOpenRomFile(wxCommandEvent&) {
  LoadCartridgeRomFile();
}

bool MainFrame::ExportCartridgeBatteryExtRam(std::string filePath) {
  if (!emulator_.IsCartridgeRomLoaded() ||
      !emulator_.GetCartridgeExtMeta().hasBattery) {
    return false;
  }

  // prompt the user for a file if empty filePath given
  if (filePath.empty()) {
    wxFileDialog saveFileDialog(this, "Export Battery-Packed RAM Snapshot",
                                wxEmptyString, wxEmptyString,
                                "Battery-Packed RAM Snapshot (*.sav)|*.sav|"
                                "All Files (*.*)|*.*",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    saveFileDialog.SetPath(emulator_.GetCartridgeRomFilePath()
                           + "EXPORTED.sav");

    if (saveFileDialog.ShowModal() == wxID_CANCEL) {
      return false;
    }

    filePath = saveFileDialog.GetPath().ToStdString();
  }

  const bool success = emulator_.ExportCartridgeBatteryExtRam(filePath);
  if (!success) {
    wxMessageDialog(this,
                    "Failed to export snapshot file! Ensure that the file is "
                    "writable and try again.",
                    "Battery-packed RAM export failed",
                    wxICON_ERROR | wxOK | wxCENTER)
        .ShowModal();
  }

  return success;
}

bool MainFrame::ImportCartridgeBatteryExtRam(std::string filePath) {
  if (!emulator_.IsCartridgeRomLoaded() ||
      !emulator_.GetCartridgeExtMeta().hasBattery) {
    return false;
  }

  wxMessageDialog confirmDialog(this,
                                "Importing a new snapshot will reset the "
                                "running program and overwrite the currently "
                                "used snapshot!\n\n"
                                "Saved data within the program (leaderboard "
                                "scores, character saves, etc.) will be "
                                "overwritten!\n\n"
                                "Consider exporting the currently used "
                                "snapshot first to avoid losing important "
                                "saved data.\n\n"
                                "Really continue?",
                                "Import Battery-Packed RAM Snapshot",
                                wxICON_EXCLAMATION | wxYES_NO | wxCENTER);

  if (confirmDialog.ShowModal() != wxID_YES) {
    return false;
  }

  // prompt the user for a file if empty filePath given
  if (filePath.empty()) {
    wxFileDialog openFileDialog(this, "Import Battery-Packed RAM Snapshot",
                                wxEmptyString, wxEmptyString,
                                "Battery-Packed RAM Snapshot (*.sav)|*.sav|"
                                "All Files (*.*)|*.*",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
      return false;
    }

    filePath = openFileDialog.GetPath().ToStdString();
  }

  const bool success = emulator_.ImportCartridgeBatteryExtRam(filePath);
  if (!success) {
    wxMessageDialog(this,
                    "Failed to import snapshot file! Ensure that the file is "
                    "readable and compatible with the loaded program and try "
                    "again.",
                    "Battery-packed RAM import failed",
                    wxICON_ERROR | wxOK | wxCENTER)
        .ShowModal();
  }

  UpdateUIState();
  return success;
}

void MainFrame::OnImportBattery(wxCommandEvent&) {
  ImportCartridgeBatteryExtRam();
}

void MainFrame::OnExportBattery(wxCommandEvent&) {
  ExportCartridgeBatteryExtRam();
}

void MainFrame::OnPause(wxCommandEvent& event) {
  emulator_.SetPaused(event.IsChecked());
  UpdateUIState();
}

void MainFrame::OnReset(wxCommandEvent&) {
  emulator_.Reset();
  UpdateUIState();
}

void MainFrame::OnResetInDmgMode(wxCommandEvent&) {
  emulator_.Reset(true);
  UpdateUIState();
}

void MainFrame::OnLimitFramerate(wxCommandEvent& event) {
  emulator_.SetLimitFramerate(event.IsChecked());
}

void MainFrame::OnEnableBg(wxCommandEvent& event) {
  emulator_.SetVideoBgRenderEnabled(event.IsChecked());
}

void MainFrame::OnEnableBgWindow(wxCommandEvent& event) {
  emulator_.SetVideoBgWindowRenderEnabled(event.IsChecked());
}

void MainFrame::OnEnableSprites(wxCommandEvent& event) {
  emulator_.SetVideoSpritesRenderEnabled(event.IsChecked());
}

void MainFrame::OnLimitScanlineSprites(wxCommandEvent& event) {
  emulator_.SetVideoScanlineSpritesLimiterEnabled(event.IsChecked());
}

void MainFrame::OnMaintainAspectRatio(wxCommandEvent& event) {
  lcdCanvas_->SetMaintainAspectRatio(event.IsChecked());
}

void MainFrame::OnSmoothenVideo(wxCommandEvent& event) {
  lcdCanvas_->SetSmoothFilterEnabled(event.IsChecked());
}

void MainFrame::OnMuteSound(wxCommandEvent& event) {
  if (event.IsChecked()) {
    audioOut_.StopStreaming();
  } else {
    audioOut_.StartStreaming();
  }
}

void MainFrame::OnMuteCh1(wxCommandEvent& event) {
  emulator_.SetApuMuteCh1(event.IsChecked());
}

void MainFrame::OnMuteCh2(wxCommandEvent& event) {
  emulator_.SetApuMuteCh2(event.IsChecked());
}

void MainFrame::OnMuteCh3(wxCommandEvent& event) {
  emulator_.SetApuMuteCh3(event.IsChecked());
}

void MainFrame::OnMuteCh4(wxCommandEvent& event) {
  emulator_.SetApuMuteCh4(event.IsChecked());
}

void MainFrame::OnJoypadImpossibleInputs(wxCommandEvent& event) {
  emulator_.SetJoypadImpossibleInputsAllowed(event.IsChecked());
}

void MainFrame::OnJoypadControls(wxCommandEvent&) {
  JoypadDialog controlsDialog(this);
  controlsDialog.CentreOnParent();
  controlsDialog.ShowModal();
}

void MainFrame::OnAbout(wxCommandEvent&) {
  AboutDialog aboutDialog(this);
  aboutDialog.CentreOnParent();
  aboutDialog.ShowModal();
}

void MainFrame::OnExit(wxCommandEvent&) {
  Close(true);
}

void MainFrame::OnClose(wxCloseEvent&) {
  Destroy();
}

void MainFrame::OnSize(wxSizeEvent&) {
  Layout();
}

bool MainFrame::HandleJoypadKeyEvent(wxKeyEvent& event) {
  const bool keyDown = event.GetEventType() == wxEVT_KEY_DOWN;

  switch (event.GetKeyCode()) {
    case 'Z':
      emulator_.SetJoypadKeyState(JoypadKey::B, keyDown);
      return true;
    case 'X':
      emulator_.SetJoypadKeyState(JoypadKey::A, keyDown);
      return true;
    case WXK_LEFT:
      emulator_.SetJoypadKeyState(JoypadKey::Left, keyDown);
      return true;
    case WXK_RIGHT:
      emulator_.SetJoypadKeyState(JoypadKey::Right, keyDown);
      return true;
    case WXK_UP:
      emulator_.SetJoypadKeyState(JoypadKey::Up, keyDown);
      return true;
    case WXK_DOWN:
      emulator_.SetJoypadKeyState(JoypadKey::Down, keyDown);
      return true;
    case WXK_SHIFT:
      emulator_.SetJoypadKeyState(JoypadKey::Select, keyDown);
      return true;
    case WXK_RETURN:
      emulator_.SetJoypadKeyState(JoypadKey::Start, keyDown);
      return true;

    default:
      event.Skip();
      return false;
  }
}

void MainFrame::OnKeyDown(wxKeyEvent& event) {
  HandleJoypadKeyEvent(event);
}

void MainFrame::OnKeyUp(wxKeyEvent& event) {
  HandleJoypadKeyEvent(event);
}

void MainFrame::RecursivelyConnectKeyEvents(wxWindow* childComponent) {
  // frames do not capture key events from their children, so to do so, we need
  // to recursively go through our children and connect events so that they
  // propogate to our frame
  //
  // implementation from https://wiki.wxwidgets.org/Catching_key_events_globally
  if (!childComponent) {
    return;
  }

  childComponent->Connect(wxID_ANY,
                          wxEVT_KEY_DOWN,
                          wxKeyEventHandler(MainFrame::OnKeyDown),
                          nullptr,
                          this);

  childComponent->Connect(wxID_ANY,
                          wxEVT_KEY_UP,
                          wxKeyEventHandler(MainFrame::OnKeyUp),
                          nullptr,
                          this);

  auto childNode = childComponent->GetChildren().GetFirst();

  while (childNode) {
    auto child = childNode->GetData();
    RecursivelyConnectKeyEvents(child);

    childNode = childNode->GetNext();
  }
}

MainFrameFileDropTarget::MainFrameFileDropTarget(MainFrame& mainFrame)
    : mainFrame_(mainFrame) {}

bool MainFrameFileDropTarget::OnDropFiles(wxCoord, wxCoord,
                                          const wxArrayString& filenames) {
  // only accept one dropped file
  if (filenames.GetCount() != 1) {
    return false;
  }

  if (wxFileName(filenames[0]).GetExt() == "sav") {
    // battery-packed ext RAM snapshot
    mainFrame_.ImportCartridgeBatteryExtRam(filenames[0].ToStdString());
  } else {
    // probably a ROM file
    mainFrame_.LoadCartridgeRomFile(filenames[0].ToStdString());
  }

  return true;
}
