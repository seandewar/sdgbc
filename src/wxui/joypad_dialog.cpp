#include "wxui/joypad_dialog.h"
#include "wxui/xpm/xpm.h"

JoypadDialog::JoypadDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Joypad Controls") {
  auto infoSizer = new wxBoxSizer(wxVERTICAL);

  infoSizer->Add(new wxStaticText(this, wxID_ANY, "Return = Start Button."),
                 0, wxCENTER);
  infoSizer->Add(new wxStaticText(this, wxID_ANY, "Shift = Select Button."),
                 0, wxCENTER);
  infoSizer->AddSpacer(15);

  infoSizer->Add(new wxStaticText(this, wxID_ANY, "Z = B Button."),
                 0, wxCENTER);
  infoSizer->Add(new wxStaticText(this, wxID_ANY, "X = A Button."),
                 0, wxCENTER);
  infoSizer->AddSpacer(15);

  infoSizer->Add(new wxStaticText(this, wxID_ANY,
                                  "Arrow Keys = Direction Buttons."),
                 0, wxCENTER);
  infoSizer->AddSpacer(10);

  auto mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(infoSizer, 0, wxALL, 16);
  mainSizer->Add(CreateButtonSizer(wxOK), 0, wxBOTTOM | wxCENTER, 16);

  SetSizerAndFit(mainSizer);
}
