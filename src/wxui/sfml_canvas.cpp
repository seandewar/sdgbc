#include "wxui/sfml_canvas.h"

#ifdef __WXGTK__
  #include <gdk/gdkx.h>
  #include <gtk/gtk.h>
#endif

wxBEGIN_EVENT_TABLE(SfmlCanvas, wxControl)
  EVT_IDLE(SfmlCanvas::OnIdle)
  EVT_PAINT(SfmlCanvas::OnPaint)
  EVT_SIZE(SfmlCanvas::OnSize)
  EVT_ERASE_BACKGROUND(SfmlCanvas::OnEraseBackground)
  EVT_SET_FOCUS(SfmlCanvas::OnSetFocus)
  EVT_KILL_FOCUS(SfmlCanvas::OnKillFocus)
wxEND_EVENT_TABLE()

SfmlCanvas::SfmlCanvas(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                       const wxSize& size, long style)
    : wxControl(parent, id, pos, size, style), isInFocus_(false) {
#ifdef __WXGTK__

  // GTK requires the X11 identifier of the widget
  // NOTE: using SFML 2.3.2 or higher causes the program to crash with an X11
  // BadAccess! not exactly sure why but its a weird incompatability
  gtk_widget_realize(m_wxwindow);
  gtk_widget_set_double_buffered(m_wxwindow, false);
  const auto window = gtk_widget_get_window(m_wxwindow);
  XFlush(GDK_WINDOW_XDISPLAY(window));
  sf::RenderWindow::create(GDK_WINDOW_XID(window));

#else

  // NOTE: not sure if this works on mac
  sf::RenderWindow::create(GetHandle());

#endif
}

void SfmlCanvas::CanvasRender() {}

bool SfmlCanvas::HasFocus() const {
  return isInFocus_;
}

void SfmlCanvas::OnIdle(wxIdleEvent&) {
  Refresh(); // send a paint msg when control is idle to ensure max frame rate
}

void SfmlCanvas::OnPaint(wxPaintEvent&) {
  wxPaintDC dc(this); // prepare control to be re-painted

  // adjust size of the sfml view to work with wxWidgets properly
  const auto canvasWxSize = GetSize();
  const auto canvasSize = sf::Vector2f(canvasWxSize.x, canvasWxSize.y);

  setView(sf::View(sf::FloatRect(0.0f, 0.0f, canvasSize.x, canvasSize.y)));

  CanvasRender();
  display();
}

void SfmlCanvas::OnSize(wxSizeEvent& event) {
  // adjust size of the sfml canvas render window to work with wxWidgets
  setSize(sf::Vector2u(event.GetSize().x, event.GetSize().y));
}

void SfmlCanvas::OnSetFocus(wxFocusEvent& event) {
  isInFocus_ = true;
  event.Skip();
}

void SfmlCanvas::OnKillFocus(wxFocusEvent& event) {
  isInFocus_ = false;
  event.Skip();
}

// do not erase what we've drawn - avoids flickering
void SfmlCanvas::OnEraseBackground(wxEraseEvent&) {}
