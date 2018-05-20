#ifndef SDGBC_SFML_CANVAS_H_
#define SDGBC_SFML_CANVAS_H_

#include "wxui/wx.h"
#include <SFML/Graphics.hpp>
#include <atomic>

// NOTE: code for this class (header & source file) is mainly derived from:
// https://www.sfml-dev.org/tutorials/1.6/graphics-wxwidgets.php
class SfmlCanvas : public wxControl, public sf::RenderWindow {
public:
  SfmlCanvas(wxWindow* parent = nullptr, wxWindowID id = wxID_ANY,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE);
  virtual ~SfmlCanvas() = default;

  // NOTE: on some platforms (such as Windows) HasFocus() will not return the
  // correct value when called from another thread! we provide our own impl
  // that fixes this issue here
  bool HasFocus() const override;

private:
  wxDECLARE_EVENT_TABLE();

  std::atomic<bool> isInFocus_;

  virtual void CanvasRender();

  void OnIdle(wxIdleEvent& event);
  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnSetFocus(wxFocusEvent& event);
  void OnKillFocus(wxFocusEvent& event);
  void OnEraseBackground(wxEraseEvent& event);
};

#endif // SDGBC_SFML_CANVAS_H_
