#include "wxui/xpm/xpm.h"
#include "wxui/about_dialog.h"
#include <SFML/Config.hpp>

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "About") {
  auto infoSizer = new wxBoxSizer(wxVERTICAL);

  infoSizer->Add(new wxStaticBitmap(this, wxID_ANY, wxBitmap(xpmSdgbc64)),
                 0, wxCENTER);
  infoSizer->AddSpacer(15);

  infoSizer->Add(new wxStaticText(this, wxID_ANY,
                                  "sdgbc - the Nintendo Game Boy Color emulator"
                                  "."),
                 0, wxCENTER);
  infoSizer->AddSpacer(15);

  infoSizer->Add(new wxStaticText(this, wxID_ANY,
                                  "Final year B.Sc. Computer Science project at"
                                  " the University of Leicester."),
                 0, wxCENTER);
  infoSizer->Add(new wxStaticText(this, wxID_ANY,
                                  "Developed by Sean Dewar, 2017-18."),
                 0, wxCENTER);
  infoSizer->AddSpacer(10);

  wxString versionText;
  versionText << "Compiled using wxWidgets " << wxMAJOR_VERSION << "."
              << wxMINOR_VERSION << " and SFML " << SFML_VERSION_MAJOR << "."
              << SFML_VERSION_MINOR << ".";
  infoSizer->Add(new wxStaticText(this, wxID_ANY, versionText), 0, wxCENTER);

  auto mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(infoSizer, 0, wxALL, 16);
  mainSizer->Add(CreateButtonSizer(wxOK), 0, wxBOTTOM | wxCENTER, 16);

  SetSizerAndFit(mainSizer);
}
