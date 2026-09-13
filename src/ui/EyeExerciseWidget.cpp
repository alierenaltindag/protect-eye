#include "EyeExerciseWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EyeExerciseWidget::EyeExerciseWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(170);
    setAttribute(Qt::WA_TranslucentBackground, true);

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        update();
    });
}

EyeExerciseWidget::~EyeExerciseWidget() {
    m_timer.stop();
}

void EyeExerciseWidget::setVisualType(ExerciseVisualType type) {
    m_type = type;
    if (m_type == ExerciseVisualType::None) {
        stopAnimation();
        setVisible(false);
    } else {
        setVisible(true);
        if (m_elapsedTimer.isValid()) {
            m_elapsedTimer.restart();
        }
        startAnimation();
    }
    update();
}

void EyeExerciseWidget::startAnimation() {
    if (!m_elapsedTimer.isValid()) {
        m_elapsedTimer.start();
    }
    if (!m_timer.isActive()) {
        m_timer.start(16); // ~60 FPS
    }
}

void EyeExerciseWidget::stopAnimation() {
    m_timer.stop();
}

void EyeExerciseWidget::paintEvent(QPaintEvent* /*event*/) {
    if (m_type == ExerciseVisualType::None) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Subtle container background rounded box
    const QRectF boxRect = rect().adjusted(4, 4, -4, -4);
    QPainterPath bgPath;
    bgPath.addRoundedRect(boxRect, 16, 16);
    painter.fillPath(bgPath, QColor(15, 23, 42, 175)); // Dark slate ~68% opacity
    painter.strokePath(bgPath, QPen(QColor(56, 189, 248, 55), 1)); // Subtle cyan border

    const qint64 elapsed = m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;

    switch (m_type) {
        case ExerciseVisualType::TrackingInfinity:
            drawLemniscate(painter, elapsed);
            break;
        case ExerciseVisualType::TrackingCircle:
            drawCircle(painter, elapsed);
            break;
        case ExerciseVisualType::TrackingHorizontal:
            drawHorizontal(painter, elapsed);
            break;
        case ExerciseVisualType::TrackingVertical:
            drawVertical(painter, elapsed);
            break;
        case ExerciseVisualType::BreathingCircle:
            drawBreathing(painter, elapsed);
            break;
        case ExerciseVisualType::DistanceFocus:
            drawDistanceFocus(painter, elapsed);
            break;
        case ExerciseVisualType::NearFarShift:
            drawNearFarShift(painter, elapsed);
            break;
        case ExerciseVisualType::SqueezeBlink:
            drawSqueezeBlink(painter, elapsed);
            break;
        case ExerciseVisualType::PeripheralExpansion:
            drawPeripheralExpansion(painter, elapsed);
            break;
        case ExerciseVisualType::None:
            break;
    }
}

void EyeExerciseWidget::drawGlowDot(QPainter& painter, const QPointF& pos, qreal radius) {
    // Outer radial glow
    const qreal glowRadius = radius * 3.0;
    QRadialGradient glow(pos, glowRadius);
    glow.setColorAt(0.0, QColor(56, 189, 248, 190));
    glow.setColorAt(0.4, QColor(56, 189, 248, 80));
    glow.setColorAt(1.0, QColor(56, 189, 248, 0));

    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(pos, glowRadius, glowRadius);

    // Inner bright core
    QRadialGradient core(pos, radius);
    core.setColorAt(0.0, QColor(240, 249, 255, 255)); // Bright white-blue
    core.setColorAt(0.7, QColor(56, 189, 248, 240));
    core.setColorAt(1.0, QColor(14, 165, 233, 220));

    painter.setBrush(core);
    painter.drawEllipse(pos, radius, radius);
}

void EyeExerciseWidget::drawLemniscate(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    const qreal a = (width() - 80) * 0.42;
    const qreal b = (height() - 40) * 0.70;

    auto evalPoint = [cx, cy, a, b](double t) -> QPointF {
        double denom = 1.0 + std::sin(t) * std::sin(t);
        double x = cx + a * std::cos(t) / denom;
        double y = cy + b * (std::sin(t) * std::cos(t)) / denom;
        return QPointF(x, y);
    };

    // Draw faint lemniscate guide path
    QPainterPath path;
    const int sampleCount = 100;
    for (int i = 0; i <= sampleCount; ++i) {
        double t = (i / static_cast<double>(sampleCount)) * 2.0 * M_PI;
        QPointF pt = evalPoint(t);
        if (i == 0) path.moveTo(pt);
        else path.lineTo(pt);
    }
    path.closeSubpath();

    painter.setPen(QPen(QColor(56, 189, 248, 45), 1.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // 6.6-second period for calm, smooth ocular pursuit (~3 full cycles in 20s)
    const double mainT = (elapsed % 6600) / 6600.0 * 2.0 * M_PI;

    // Draw trailing ghost dots
    for (int trail = 4; trail >= 1; --trail) {
        const double trailT = ((elapsed - trail * 60) % 6600) / 6600.0 * 2.0 * M_PI;
        const QPointF trailPos = evalPoint(trailT);
        const qreal trailAlpha = 0.18 - (trail * 0.035);
        const qreal r = 7.0 - (trail * 0.8);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(56, 189, 248, static_cast<int>(trailAlpha * 255)));
        painter.drawEllipse(trailPos, r, r);
    }

    // Main tracking dot
    drawGlowDot(painter, evalPoint(mainT), 8.5);
}

void EyeExerciseWidget::drawCircle(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    const qreal r = qMin(width(), height()) * 0.36;

    // Draw faint circle guide
    painter.setPen(QPen(QColor(56, 189, 248, 45), 1.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(cx, cy), r, r);

    // 6.6-second period (~3 full cycles in 20s)
    const double mainT = (elapsed % 6600) / 6600.0 * 2.0 * M_PI;

    auto evalPoint = [cx, cy, r](double t) -> QPointF {
        return QPointF(cx + r * std::cos(t), cy + r * std::sin(t));
    };

    // Ghost trails
    for (int trail = 4; trail >= 1; --trail) {
        const double trailT = ((elapsed - trail * 60) % 6600) / 6600.0 * 2.0 * M_PI;
        const QPointF trailPos = evalPoint(trailT);
        const qreal trailAlpha = 0.18 - (trail * 0.035);
        const qreal tr = 7.0 - (trail * 0.8);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(56, 189, 248, static_cast<int>(trailAlpha * 255)));
        painter.drawEllipse(trailPos, tr, tr);
    }

    drawGlowDot(painter, evalPoint(mainT), 8.5);
}

void EyeExerciseWidget::drawHorizontal(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    const qreal span = (width() - 80) * 0.44;

    // Faint horizontal guide line with end ticks
    painter.setPen(QPen(QColor(56, 189, 248, 45), 1.5, Qt::DashLine));
    painter.drawLine(QPointF(cx - span, cy), QPointF(cx + span, cy));

    painter.setPen(QPen(QColor(56, 189, 248, 60), 2));
    painter.drawLine(QPointF(cx - span, cy - 8), QPointF(cx - span, cy + 8));
    painter.drawLine(QPointF(cx + span, cy - 8), QPointF(cx + span, cy + 8));

    // 5-second period (2.5s each way, exactly 4 full cycles in 20s)
    const double mainT = (elapsed % 5000) / 5000.0 * 2.0 * M_PI;
    const qreal mainX = cx + span * std::sin(mainT);

    // Ghost trails
    for (int trail = 4; trail >= 1; --trail) {
        const double trailT = ((elapsed - trail * 60) % 5000) / 5000.0 * 2.0 * M_PI;
        const qreal trailX = cx + span * std::sin(trailT);
        const qreal trailAlpha = 0.18 - (trail * 0.035);
        const qreal tr = 7.0 - (trail * 0.8);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(56, 189, 248, static_cast<int>(trailAlpha * 255)));
        painter.drawEllipse(QPointF(trailX, cy), tr, tr);
    }

    drawGlowDot(painter, QPointF(mainX, cy), 8.5);
}

void EyeExerciseWidget::drawBreathing(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;

    // 8-second breathing loop (4s Inhale, 4s Exhale)
    const double phase = (elapsed % 8000) / 8000.0;
    const bool isInhale = (phase < 0.5);
    const double subFrac = isInhale ? (phase / 0.5) : ((phase - 0.5) / 0.5);

    // Smooth sinusoidal ease
    const double eased = isInhale
        ? (1.0 - std::cos(subFrac * M_PI)) / 2.0
        : (1.0 + std::cos(subFrac * M_PI)) / 2.0;

    const qreal minRadius = 30.0;
    const qreal maxRadius = 66.0;
    const qreal currentRadius = minRadius + eased * (maxRadius - minRadius);

    // Pulsing aura gradient
    QRadialGradient aura(QPointF(cx, cy), currentRadius * 1.35);
    if (isInhale) {
        aura.setColorAt(0.0, QColor(45, 212, 191, 140)); // Emerald / teal
        aura.setColorAt(0.6, QColor(56, 189, 248, 60));
        aura.setColorAt(1.0, QColor(56, 189, 248, 0));
    } else {
        aura.setColorAt(0.0, QColor(56, 189, 248, 140)); // Sky blue
        aura.setColorAt(0.6, QColor(99, 102, 241, 60));
        aura.setColorAt(1.0, QColor(99, 102, 241, 0));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(aura);
    painter.drawEllipse(QPointF(cx, cy), currentRadius * 1.35, currentRadius * 1.35);

    // Inner smooth ring
    QPen ringPen(isInhale ? QColor(45, 212, 191, 220) : QColor(56, 189, 248, 220), 2.5);
    painter.setPen(ringPen);
    painter.setBrush(QColor(15, 23, 42, 190));
    painter.drawEllipse(QPointF(cx, cy), currentRadius, currentRadius);

    // Text inside ring (Inhale / Exhale)
    QFont font = painter.font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(isInhale ? QColor(153, 246, 228) : QColor(186, 230, 253));

    const QString breathText = isInhale
        ? Localization::instance().breathInhale()
        : Localization::instance().breathExhale();

    painter.drawText(QRectF(cx - 80, cy - 14, 160, 28), Qt::AlignCenter, breathText);
}

void EyeExerciseWidget::drawDistanceFocus(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;

    // Expanding calm waves into distance (3.5s loop)
    const double loopTime = (elapsed % 3500) / 3500.0;

    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < 3; ++i) {
        double offset = loopTime + (i / 3.0);
        if (offset > 1.0) offset -= 1.0;

        qreal waveRadius = 18.0 + offset * 62.0;
        int alpha = static_cast<int>((1.0 - offset) * 120);

        painter.setPen(QPen(QColor(56, 189, 248, alpha), 1.8));
        painter.drawEllipse(QPointF(cx, cy), waveRadius, waveRadius);
    }

    // Central tranquil icon / point
    drawGlowDot(painter, QPointF(cx, cy), 7.0);

    // Label below
    QFont font = painter.font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(148, 163, 184));
    painter.drawText(QRectF(cx - 100, cy + 32, 200, 24), Qt::AlignCenter, QStringLiteral("6m / 20ft → ∞"));
}

void EyeExerciseWidget::drawVertical(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    const qreal span = (height() - 40) * 0.40;

    // Faint vertical guide line with end ticks
    painter.setPen(QPen(QColor(56, 189, 248, 45), 1.5, Qt::DashLine));
    painter.drawLine(QPointF(cx, cy - span), QPointF(cx, cy + span));

    painter.setPen(QPen(QColor(56, 189, 248, 60), 2));
    painter.drawLine(QPointF(cx - 8, cy - span), QPointF(cx + 8, cy - span));
    painter.drawLine(QPointF(cx - 8, cy + span), QPointF(cx + 8, cy + span));

    // 5-second period (2.5s each way, exactly 4 full cycles in 20s)
    const double mainT = (elapsed % 5000) / 5000.0 * 2.0 * M_PI;
    const qreal mainY = cy + span * std::sin(mainT);

    // Ghost trails
    for (int trail = 4; trail >= 1; --trail) {
        const double trailT = ((elapsed - trail * 60) % 5000) / 5000.0 * 2.0 * M_PI;
        const qreal trailY = cy + span * std::sin(trailT);
        const qreal trailAlpha = 0.18 - (trail * 0.035);
        const qreal tr = 7.0 - (trail * 0.8);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(56, 189, 248, static_cast<int>(trailAlpha * 255)));
        painter.drawEllipse(QPointF(cx, trailY), tr, tr);
    }

    drawGlowDot(painter, QPointF(cx, mainY), 8.5);
}

void EyeExerciseWidget::drawNearFarShift(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;

    // 6-second cycle: 3s Near, 3s Far
    const double phase = (elapsed % 6000) / 6000.0;
    const bool isNear = (phase < 0.5);
    const double subFrac = isNear ? (phase / 0.5) : ((phase - 0.5) / 0.5);
    const double eased = (1.0 - std::cos(subFrac * M_PI)) / 2.0;

    if (isNear) {
        // Near focus target: vibrant focal circle pulsing
        const qreal r = 26.0 + eased * 6.0;
        QRadialGradient glow(QPointF(cx, cy - 8), r * 1.8);
        glow.setColorAt(0.0, QColor(56, 189, 248, 180));
        glow.setColorAt(0.6, QColor(56, 189, 248, 60));
        glow.setColorAt(1.0, QColor(56, 189, 248, 0));

        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(QPointF(cx, cy - 8), r * 1.8, r * 1.8);

        painter.setPen(QPen(QColor(56, 189, 248, 230), 2.5));
        painter.setBrush(QColor(15, 23, 42, 200));
        painter.drawEllipse(QPointF(cx, cy - 8), r, r);

        drawGlowDot(painter, QPointF(cx, cy - 8), 6.5);
    } else {
        // Far focus: expanding ripples drifting away
        painter.setBrush(Qt::NoBrush);
        for (int i = 0; i < 3; ++i) {
            double offset = subFrac + (i / 3.0);
            if (offset > 1.0) offset -= 1.0;
            qreal waveR = 20.0 + offset * 60.0;
            int alpha = static_cast<int>((1.0 - offset) * 130);
            painter.setPen(QPen(QColor(45, 212, 191, alpha), 1.8));
            painter.drawEllipse(QPointF(cx, cy - 8), waveR, waveR);
        }
        drawGlowDot(painter, QPointF(cx, cy - 8), 5.5);
    }

    // Guidance label
    QFont font = painter.font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(isNear ? QColor(186, 230, 253) : QColor(153, 246, 228));

    const QString label = isNear
        ? Localization::instance().focusNearLabel()
        : Localization::instance().focusFarLabel();

    painter.drawText(QRectF(cx - 160, cy + 34, 320, 26), Qt::AlignCenter, label);
}

void EyeExerciseWidget::drawSqueezeBlink(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;

    // 6-second cycle: 2s Close, 2s Squeeze, 2s Open
    const int stage = (elapsed % 6000) / 2000; // 0, 1, 2
    const double stageFrac = ((elapsed % 2000) / 2000.0);
    const double pulse = std::sin(stageFrac * M_PI);

    QString phaseText;
    QColor themeColor;

    if (stage == 0) {
        // Close
        phaseText = Localization::instance().squeezePhaseClose();
        themeColor = QColor(148, 163, 184); // Soft slate

        // Draw gentle closed eye curve
        QPainterPath eyePath;
        eyePath.moveTo(cx - 30, cy - 10);
        eyePath.quadTo(cx, cy + 4, cx + 30, cy - 10);
        painter.setPen(QPen(themeColor, 3, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(eyePath);
    } else if (stage == 1) {
        // Squeeze
        phaseText = Localization::instance().squeezePhaseSqueeze();
        themeColor = QColor(56, 189, 248); // Glowing cyan

        // Draw squeezed eye curve with eyelash rays
        QPainterPath eyePath;
        eyePath.moveTo(cx - 30, cy - 8);
        eyePath.quadTo(cx, cy + 8 + pulse * 3.0, cx + 30, cy - 8);
        painter.setPen(QPen(themeColor, 3.5, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(eyePath);

        // Squeeze rays
        painter.setPen(QPen(QColor(56, 189, 248, 160), 2, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(cx - 16, cy + 12), QPointF(cx - 22, cy + 20));
        painter.drawLine(QPointF(cx, cy + 14), QPointF(cx, cy + 23));
        painter.drawLine(QPointF(cx + 16, cy + 12), QPointF(cx + 22, cy + 20));
    } else {
        // Open & Relax
        phaseText = Localization::instance().squeezePhaseOpen();
        themeColor = QColor(45, 212, 191); // Soothing emerald

        // Draw open eye almond shape
        QPainterPath openPath;
        openPath.moveTo(cx - 30, cy - 8);
        openPath.quadTo(cx, cy - 24, cx + 30, cy - 8);
        openPath.quadTo(cx, cy + 8, cx - 30, cy - 8);
        painter.setPen(QPen(themeColor, 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(QColor(15, 23, 42, 180));
        painter.drawPath(openPath);

        // Iris pupil
        drawGlowDot(painter, QPointF(cx, cy - 8), 6.0);
    }

    // Phase indicator dots (1, 2, 3)
    for (int i = 0; i < 3; ++i) {
        const qreal dotX = cx - 24 + i * 24;
        const qreal dotY = cy + 28;
        if (i == stage) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(themeColor);
            painter.drawEllipse(QPointF(dotX, dotY), 4.5, 4.5);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(71, 85, 105, 160));
            painter.drawEllipse(QPointF(dotX, dotY), 3.0, 3.0);
        }
    }

    // Label
    QFont font = painter.font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(themeColor);
    painter.drawText(QRectF(cx - 140, cy + 38, 280, 24), Qt::AlignCenter, phaseText);
}

void EyeExerciseWidget::drawPeripheralExpansion(QPainter& painter, qint64 elapsed) {
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    const qreal maxSpan = (width() - 80) * 0.44;

    // Concentric horizontal waves flowing outward from center to peripheries
    const double loopTime = (elapsed % 3000) / 3000.0;

    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < 3; ++i) {
        double offset = loopTime + (i / 3.0);
        if (offset > 1.0) offset -= 1.0;
        qreal currentX = offset * maxSpan;
        int alpha = static_cast<int>((1.0 - offset) * 110);

        painter.setPen(QPen(QColor(56, 189, 248, alpha), 2, Qt::SolidLine, Qt::RoundCap));
        // Left bracket wave
        painter.drawArc(QRectF(cx - currentX - 16, cy - 30, 32, 44), 90 * 16, 180 * 16);
        // Right bracket wave
        painter.drawArc(QRectF(cx + currentX - 16, cy - 30, 32, 44), -90 * 16, 180 * 16);
    }

    // Center foveal anchor dot
    drawGlowDot(painter, QPointF(cx, cy - 8), 7.5);

    // Label below
    QFont font = painter.font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(186, 230, 253));
    painter.drawText(QRectF(cx - 180, cy + 34, 360, 24), Qt::AlignCenter, QStringLiteral("⟵  👁️  ⟶"));
}
