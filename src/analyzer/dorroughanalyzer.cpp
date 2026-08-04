#include "dorroughanalyzer.h"
#include <QPainter>
#include <QtMath>
#include "fht.h"

const char *DorroughAnalyzer::kName = QT_TRANSLATE_NOOP("AnalyzerContainer", "Dorrough Model 40-A");

DorroughAnalyzer::DorroughAnalyzer(QWidget *parent)
    : AnalyzerBase(parent) {
    resizeForBands(40);
}

void DorroughAnalyzer::transform(Scope &s) {
    fht_->spectrum(s.data());
    fht_->scale(s.data(), 1.0f / 20.0f);
}

void DorroughAnalyzer::analyze(QPainter &p, const Scope &scope, bool new_frame) {
    Q_UNUSED(new_frame)

    QRect r = rect();
    p.fillRect(r, QColor(0x0A, 0x0A, 0x0A));

    float sum = 0.0f;
    float peak = 0.0f;

    for (float v : scope) {
        sum += v * v;
        peak = qMax(peak, v);
    }

    float rms = qSqrt(sum / scope.size());

    float rms_db  = 20.0f * log10f(qMax(rms, 1e-5f)) + 18.0f;
    float peak_db = 20.0f * log10f(qMax(peak, 1e-5f)) + 18.0f;

    m_persist = m_persist * 0.92f + rms_db * 0.08f;

    if (peak_db > m_peak)
        m_peak = peak_db;
    else
        m_peak *= 0.998f;

    if (peak_db > m_peakHold) {
        m_peakHold = peak_db;
        m_peakHoldCounter = 30;
    } else if (--m_peakHoldCounter <= 0) {
        m_peakHold *= 0.999f;
    }

    auto dbToLed = [](float db) -> int {
        int led = int((db + 25.0f) * (40.0f / 39.0f));
        return qBound(0, led, 39);
    };

    int persistLed = dbToLed(m_persist);
    int peakLed    = dbToLed(m_peak);
    int holdLed    = dbToLed(m_peakHold);

    const int numLeds = 40;

    // Barra recta horizontal
    const float left  = r.left() + 15.0f;
    const float right = r.right() - 15.0f;
    const float y     = r.center().y();

    const float spacing = (right - left) / float(numLeds - 1);

    for (int i = 0; i < numLeds; ++i) {

        float x = left + i * spacing;

        QRectF ledRect(x - 4.0f, y - 10.0f, 8.0f, 18.0f);

        bool on = (i <= persistLed);
        bool isPeak = (i == peakLed || i == holdLed);

        QColor col;

        float db = -25.0f + i * (39.0f / 40.0f);

        if (db < -12)
            col = QColor(0x00, 0xE0, 0x40);
        else if (db < -2)
            col = QColor(0xFF, 0xE6, 0x00);
        else if (db <= 0)
            col = QColor(0xFF, 0x10, 0x30);
        else if (db < 12)
            col = QColor(0xFF, 0xE6, 0x00);
        else
            col = QColor(0xFF, 0x10, 0x30);

        if (!on && !isPeak)
            col = QColor(0x1A, 0x1A, 0x1A);

        if (on || isPeak) {
            p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(ledRect.adjusted(-2, -2, 2, 2), 2, 2);

            p.setBrush(col);
            p.drawRoundedRect(ledRect, 2, 2);
        } else {
            p.setBrush(col);
            p.setPen(Qt::NoPen);
            p.drawRect(ledRect);
        }
    }

    p.setPen(QColor(0xC0, 0xA0, 0x30));
    p.setFont(QFont(QStringLiteral("Sans"), 7));

    p.drawText(
        QRect(r.x(), r.bottom() - 18, r.width() / 2, 10),
        Qt::AlignCenter,
        QStringLiteral("NORMAL PERSISTENCE RANGE"));

    p.drawText(
        QRect(r.x() + r.width() / 2, r.bottom() - 18, r.width() / 2, 10),
        Qt::AlignCenter,
        QStringLiteral("NORMAL PEAK RANGE"));

    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("Sans"), 8, QFont::Bold));

    p.drawText(
        QRect(r.x(), r.y() + r.height() - 20, r.width(), 20),
        Qt::AlignCenter,
        QStringLiteral("Loudness Monitor  dorrough"));
}
