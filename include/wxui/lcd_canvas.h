#ifndef SDGBC_LCD_CANVAS_H_
#define SDGBC_LCD_CANVAS_H_

#include "hw/ppu.h"
#include "wxui/sfml_canvas.h"
#include <array>
#include <atomic>
#include <mutex>

class LcdCanvas : public SfmlCanvas, public ILcd {
public:
  LcdCanvas(wxWindow* parent = nullptr, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE);

  void LcdPower(bool powerOn) override;
  void LcdRefresh() override;
  void LcdPutPixel(unsigned int x, unsigned int y,
                   const RgbColor& color) override;

  void SetMaintainAspectRatio(bool val);
  bool IsMaintainingAspectRatio() const;

  void SetSmoothFilterEnabled(bool val);
  bool IsSmoothFilterEnabled() const;

private:
  bool maintainAspectRatio_;
  bool isLcdOn_;

  std::array<sf::Image, 2> lcdFrameBuffers_;
  sf::Image* lcdFrontBuffer_;
  sf::Image* lcdBackBuffer_;
  // NOTE: no need to ever lock for back buffer accesses, as these are only
  // done by the emulation thread. only the front buffer is accessed by both
  // the emulation and main thread
  mutable std::mutex lcdFrontBufferMutex_;

  std::atomic<bool> lcdTextureNeedsRefresh_;
  sf::Texture lcdTexture_;

  void CanvasRender() override;
  void ClearToWhite();
};

#endif // SDGBC_LCD_CANVAS_H_
