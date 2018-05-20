#include "wxui/lcd_canvas.h"

LcdCanvas::LcdCanvas(wxWindow* parent, wxWindowID id, const wxPoint& pos,
                     const wxSize& size, long style)
    : SfmlCanvas(parent, id, pos, size, style),
      lcdFrontBuffer_(&lcdFrameBuffers_[0]),
      lcdBackBuffer_(&lcdFrameBuffers_[1]),
      lcdTextureNeedsRefresh_(false), maintainAspectRatio_(true),
      isLcdOn_(false) {
  setFramerateLimit(60);
  lcdTexture_.setSmooth(true);

  for (auto& b : lcdFrameBuffers_) {
    b.create(kLcdWidthPixels, kLcdHeightPixels);
  }
}

void LcdCanvas::CanvasRender() {
  clear();

  // save current view & create a view scaling the LCD to fit the canvas
  const auto prevView = getView();
  sf::View newView(sf::FloatRect(0.0f, 0.0f,
                                 kLcdWidthPixels, kLcdHeightPixels));

  if (maintainAspectRatio_) {
    const auto prevRatio = prevView.getSize().x / prevView.getSize().y,
               targetRatio = newView.getSize().x / newView.getSize().y;

    if (prevRatio > targetRatio) {
      // view is too wide
      newView.setSize(newView.getSize().x * (prevRatio / targetRatio),
                      newView.getSize().y);
    } else {
      // view is too tall
      newView.setSize(newView.getSize().x,
                      newView.getSize().y * (targetRatio / prevRatio));
    }
  }

  setView(newView);

  // refresh the texture if a new frame was just finished
  if (lcdTextureNeedsRefresh_) {
    std::unique_lock<std::mutex> lock(lcdFrontBufferMutex_);
    lcdTexture_.loadFromImage(*lcdFrontBuffer_);
    lcdTextureNeedsRefresh_ = false;
  }

  draw(sf::Sprite(lcdTexture_));

  // restore the previous view
  setView(prevView);
}

void LcdCanvas::LcdRefresh() {
  std::unique_lock<std::mutex> lockFront(lcdFrontBufferMutex_);
  std::swap(lcdFrontBuffer_, lcdBackBuffer_);
  lcdTextureNeedsRefresh_ = true;
}

void LcdCanvas::ClearToWhite() {
  lcdBackBuffer_->create(kLcdWidthPixels, kLcdHeightPixels,
                         sf::Color(0xff, 0xff, 0xff));
  {
    std::unique_lock<std::mutex> lockFront(lcdFrontBufferMutex_);
    lcdFrontBuffer_->create(kLcdWidthPixels, kLcdHeightPixels,
                            sf::Color(0xff, 0xff, 0xff));
  }

  LcdRefresh();
}

void LcdCanvas::LcdPutPixel(unsigned int x, unsigned int y,
                            const RgbColor& color) {
  if (x < kLcdWidthPixels && y < kLcdHeightPixels) {
    lcdBackBuffer_->setPixel(x, y, sf::Color(color.r, color.g, color.b));
  }
}

void LcdCanvas::LcdPower(bool powerOn) {
  isLcdOn_ = powerOn;

  if (!isLcdOn_) {
    ClearToWhite();
  }
}

void LcdCanvas::SetMaintainAspectRatio(bool val) {
  maintainAspectRatio_ = val;
}

bool LcdCanvas::IsMaintainingAspectRatio() const {
  return maintainAspectRatio_;
}

void LcdCanvas::SetSmoothFilterEnabled(bool val) {
  lcdTexture_.setSmooth(val);
}

bool LcdCanvas::IsSmoothFilterEnabled() const {
  return lcdTexture_.isSmooth();
}
