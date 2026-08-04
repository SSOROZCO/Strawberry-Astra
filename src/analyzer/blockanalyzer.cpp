#include "blockanalyzer.h"
#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QPalette>
#include <QColor>
#include <QLinearGradient>

namespace {
constexpr int kHeight = 4;
constexpr int kWidth = 10;
constexpr int kMinRows = 4;
constexpr int kMinColumns = 32;
constexpr int kMaxColumns = 256;
constexpr int kFadeSize = 90;
}

const char *BlockAnalyzer::kName = QT_TRANSLATE_NOOP("AnalyzerContainer", "Block analyzer");

BlockAnalyzer::BlockAnalyzer(QWidget *parent)
    : AnalyzerBase(parent, 9),
      columns_(0), rows_(0), y_(0),
      barpixmap_(1, 1), topbarpixmap_(kWidth, kHeight),
      scope_(kMinColumns), store_(1 << 8, 0),
      fade_bars_(kFadeSize), fade_pos_(1 << 8, 50),
      fade_intensity_(1 << 8, 32), step_(0) {
  setMinimumSize(kMinColumns * (kWidth + 1) - 1, kMinRows * (kHeight + 1) - 1);
  setMaximumWidth(kMaxColumns * (kWidth + 1) - 1);
  std::fill(fade_bars_.begin(), fade_bars_.end(), QPixmap(1, 1));
}

void BlockAnalyzer::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  background_ = QPixmap(size());
  canvas_ = QPixmap(size());
  const int oldRows = rows_;
  columns_ = qMin(static_cast<int>(static_cast<double>(width() + 1) / (kWidth + 1)) + 1, kMaxColumns);
  rows_ = static_cast<int>(static_cast<double>(height() + 1) / (kHeight + 1));
  y_ = (height() - (rows_ * (kHeight + 1)) + 2) / 2;
  scope_.resize(static_cast<size_t>(columns_));
  if (rows_!= oldRows) {
    barpixmap_ = QPixmap(kWidth, rows_ * (kHeight + 1));
    std::fill(fade_bars_.begin(), fade_bars_.end(), QPixmap(kWidth, rows_ * (kHeight + 1)));
    yscale_.resize(rows_ + 1);
    const int PRE = 1, PRO = 1;
    for (int z = 0; z < rows_; ++z) yscale_[z] = 1 - (log10(PRE + z) / log10(PRE + rows_ + PRO));
    yscale_[rows_] = 0;
    determineStep();
    paletteChange(palette());
  }
  drawBackground();
}

void BlockAnalyzer::determineStep() {
  const double fallTime = static_cast<double>(timeout() < 20? 20 * rows_ : 30 * rows_);
  step_ = static_cast<double>(rows_ * timeout()) / fallTime;
}
void BlockAnalyzer::framerateChanged() { determineStep(); }

void BlockAnalyzer::transform(Scope &s) {
  for (uint x = 0; x < s.size(); ++x) s[x] *= 2;
  fht_->spectrum(s.data());
  fht_->scale(s.data(), 1.0F / 20);
  s.resize(scope_.size() <= kMaxColumns / 2? kMaxColumns / 2 : scope_.size());
}

void BlockAnalyzer::analyze(QPainter &p, const Scope &s, const bool new_frame) {
  if (!new_frame) { p.drawPixmap(0, 0, canvas_); return; }
  QPainter canvas_painter(&canvas_);
  interpolate(s, scope_);
  canvas_painter.drawPixmap(0, 0, background_);
  for (qint64 x = 0, y = 0; x < static_cast<qint64>(scope_.size()); ++x) {
    for (y = 0; y < rows_ && scope_[static_cast<quint64>(x)] < yscale_.at(y); ++y);
    if (static_cast<double>(y) > store_.at(x)) { store_[x] += step_; y = static_cast<int>(store_.value(x)); }
    else store_[x] = static_cast<double>(y);
    if (y <= fade_pos_.at(x)) { fade_pos_[x] = static_cast<int>(y); fade_intensity_[x] = kFadeSize; }
    if (fade_intensity_.at(x) > 0) {
      --fade_intensity_[x];
      const int offset = fade_intensity_.value(x);
      const int y2 = y_ + (fade_pos_.value(x) * (kHeight + 1));
      canvas_painter.drawPixmap(static_cast<int>(x) * (kWidth + 1), y2, fade_bars_[offset], 0, 0, kWidth, height() - y2);
    }
    if (fade_intensity_.at(x) == 0) fade_pos_[x] = rows_;
    canvas_painter.drawPixmap(static_cast<int>(x) * (kWidth + 1), static_cast<int>(y) * (kHeight + 1) + y_, *bar(), 0, static_cast<int>(y) * (kHeight + 1), bar()->width(), bar()->height());
  }
  for (int x = 0; x < store_.size(); ++x) {
    canvas_painter.drawPixmap(x * (kWidth + 1), static_cast<int>(store_[x]) * (kHeight + 1) + y_, topbarpixmap_);
  }
  p.drawPixmap(0, 0, canvas_);
}

// --- ESTILO 80s/90s CORREGIDO ---
void BlockAnalyzer::paletteChange(const QPalette &) {
  topbarpixmap_.fill(QColor(255, 240, 180));

  auto retroColor = [](double t) -> QColor {
    if (t < 0.35) {
      double k = t / 0.35;
      return QColor(int(255*(1-k)+255*k), int(232*(1-k)+0*k), int(0*(1-k)+170*k));
    } else if (t < 0.70) {
      double k = (t-0.35)/0.35;
      return QColor(int(255*(1-k)+0*k), int(0*(1-k)+255*k), int(170*(1-k)+224*k));
    } else {
      double k = (t-0.70)/0.30;
      return QColor(int(0*(1-k)+0*k), int(255*(1-k)+160*k), int(224*(1-k)+255*k));
    }
  };

  bar()->fill(Qt::transparent);
  QPainter p(bar());
  for (int y = 0; y < rows_; ++y) {
    int yy = y * (kHeight + 1);
    QColor c = retroColor(double(y)/double(rows_));
    p.fillRect(0, yy, kWidth, kHeight, c);
    p.fillRect(0, yy + kHeight - 1, kWidth, 1, QColor(0,0,0,80));
  }

  for (int y = 0; y < kFadeSize; ++y) {
    fade_bars_[y].fill(Qt::transparent);
    QPainter f(&fade_bars_[y]);
    double alpha = static_cast<double>(y) / kFadeSize;
    for (int z = 0; z < rows_; ++z) {
      QColor c = retroColor(double(z)/double(rows_));
      c.setAlpha(int(200 * alpha));
      f.fillRect(0, z * (kHeight + 1), kWidth, kHeight, c);
    }
  }
  drawBackground();
}

void BlockAnalyzer::drawBackground() {
  if (background_.isNull()) return;
  background_.fill(QColor(0x0A, 0x0A, 0x0F));
  QPainter p(&background_);
  if (!p.paintEngine()) return;
  QColor grid(0x30, 0x20, 0x50, 60);
  for (int x = 0; x < columns_; ++x) {
    for (int y = 0; y < rows_; ++y) {
      int px = x * (kWidth + 1);
      int py = y * (kHeight + 1) + y_;
      p.fillRect(px, py, kWidth, kHeight, QColor(0x14,0x14,0x1E));
      p.setPen(grid);
      p.drawRect(px, py, kWidth, kHeight);
    }
  }
}
