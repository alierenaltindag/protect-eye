#include "OverlayWindow.h"
#include "EyeExerciseWidget.h"
#include "core/Localization.h"
#include "core/Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QWindow>
#include <QTimer>
#include <QGraphicsDropShadowEffect>


OverlayWindow::OverlayWindow(QScreen* targetScreen, QWidget* parent)
    : QWidget(parent)
    , m_screen(targetScreen) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setAttribute(Qt::WA_AlwaysStackOnTop, true);

    setupUi();
    applyStyles();

    if (m_screen) {
        setScreen(m_screen);
        setupScreenGeometry();
        connect(m_screen, &QScreen::geometryChanged, this, [this](const QRect& geom) {
            if (!isFullScreen()) {
                setGeometry(geom);
                if (windowHandle()) {
                    windowHandle()->setGeometry(geom);
                }
            }
        });
    }
}

void OverlayWindow::setupScreenGeometry() {
    if (!m_screen) return;
    setScreen(m_screen);
    if (!isFullScreen()) {
        setGeometry(m_screen->geometry());
    }
    winId();
    if (windowHandle()) {
        windowHandle()->setScreen(m_screen);
        if (!isFullScreen()) {
            windowHandle()->setGeometry(m_screen->geometry());
        }
    }
}

void OverlayWindow::present(bool shouldActivate) {
    setupScreenGeometry();
    showFullScreen();
    raise();
    if (shouldActivate) {
        activateWindow();
        if (windowHandle()) {
            windowHandle()->requestActivate();
        }
    }

    // Guard against focus theft or click race conditions while the window is mapping
    QTimer::singleShot(50, this, [this, shouldActivate]() {
        raise();
        if (shouldActivate) {
            activateWindow();
            if (windowHandle()) {
                windowHandle()->requestActivate();
            }
        }
    });
    QTimer::singleShot(150, this, [this, shouldActivate]() {
        raise();
        if (shouldActivate) {
            activateWindow();
            if (windowHandle()) {
                windowHandle()->requestActivate();
            }
        }
    });
}

OverlayWindow::~OverlayWindow() {
    if (m_progressAnim) {
        m_progressAnim->stop();
    }
    if (m_exerciseWidget) {
        m_exerciseWidget->stopAnimation();
    }
}

void OverlayWindow::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Center card container
    auto* cardWidget = new QWidget(this);
    cardWidget->setObjectName("cardWidget");
    cardWidget->setFixedWidth(660);

    auto* cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setSpacing(16);
    cardLayout->setContentsMargins(40, 36, 40, 36);

    // Badge (Kısa Mola / Uzun Mola / Egzersiz Adı)
    m_badgeLabel = new QLabel(Localization::instance().badgeShortBreak(), cardWidget);
    m_badgeLabel->setObjectName("badgeLabel");
    m_badgeLabel->setAlignment(Qt::AlignCenter);

    QFont badgeFont = m_badgeLabel->font();
    const QString langCode = Localization::instance().effectiveLanguageCode();
    if (langCode != "ar") {
        badgeFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    }
    badgeFont.setBold(true);
    m_badgeLabel->setFont(badgeFont);

    auto* badgeLayout = new QHBoxLayout();
    badgeLayout->setContentsMargins(0, 0, 0, 0);
    badgeLayout->setAlignment(Qt::AlignCenter);
    badgeLayout->addWidget(m_badgeLabel);

    // Exercise Title
    m_titleLabel = new QLabel(cardWidget);
    m_titleLabel->setObjectName("exerciseTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);

    // Interactive Animation Widget
    m_exerciseWidget = new EyeExerciseWidget(cardWidget);
    m_exerciseWidget->setObjectName("exerciseWidget");

    // Large Countdown
    m_countdownLabel = new QLabel(QStringLiteral("15"), cardWidget);
    m_countdownLabel->setObjectName("countdownLabel");
    m_countdownLabel->setAlignment(Qt::AlignCenter);

    // Progress Bar
    m_progressBar = new QProgressBar(cardWidget);
    m_progressBar->setObjectName("breakProgressBar");
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);

    // Tip / Instruction Label
    m_tipLabel = new QLabel(cardWidget);
    m_tipLabel->setObjectName("tipLabel");
    m_tipLabel->setWordWrap(true);
    m_tipLabel->setAlignment(Qt::AlignCenter);

    // Action Buttons Layout
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(16);
    btnLayout->setAlignment(Qt::AlignCenter);

    const int defaultSnoozeMin = std::max(1, Settings::instance().snoozeDurationSec() / 60);
    m_snoozeBtn = new QPushButton(Localization::instance().buttonSnooze(defaultSnoozeMin), cardWidget);
    m_snoozeBtn->setObjectName("snoozeBtn");
    m_snoozeBtn->setCursor(Qt::PointingHandCursor);

    m_skipBtn = new QPushButton(Localization::instance().buttonSkip(), cardWidget);
    m_skipBtn->setObjectName("skipBtn");
    m_skipBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addWidget(m_snoozeBtn);
    btnLayout->addWidget(m_skipBtn);

    cardLayout->addLayout(badgeLayout);
    cardLayout->addWidget(m_titleLabel);
    cardLayout->addWidget(m_exerciseWidget);
    cardLayout->addWidget(m_countdownLabel);
    cardLayout->addWidget(m_progressBar);
    cardLayout->addWidget(m_tipLabel);
    cardLayout->addSpacing(8);
    cardLayout->addLayout(btnLayout);

    mainLayout->addWidget(cardWidget);

    connect(m_skipBtn, &QPushButton::clicked, this, [this]() {
        m_skipBtn->setEnabled(false);
        m_snoozeBtn->setEnabled(false);
        emit skipRequested();
    });
    connect(m_snoozeBtn, &QPushButton::clicked, this, [this]() {
        m_skipBtn->setEnabled(false);
        m_snoozeBtn->setEnabled(false);
        emit snoozeRequested();
    });
}

void OverlayWindow::applyStyles() {
    setStyleSheet(R"(
        QWidget#cardWidget {
            background-color: rgba(22, 27, 34, 0.90);
            border: 1px solid rgba(56, 189, 248, 0.30);
            border-radius: 24px;
        }
        QLabel#badgeLabel {
            font-size: 13px;
            font-weight: bold;
            color: #38bdf8;
            padding: 6px 22px;
            background-color: rgba(56, 189, 248, 0.12);
            border: 1px solid rgba(56, 189, 248, 0.28);
            border-radius: 12px;
        }
        QLabel#exerciseTitleLabel {
            font-size: 21px;
            font-weight: 700;
            color: #f8fafc;
            margin-top: -4px;
            margin-bottom: 2px;
        }
        QLabel#countdownLabel {
            font-size: 84px;
            font-weight: 700;
            color: #f8fafc;
            margin-top: -10px;
            margin-bottom: -10px;
        }
        QProgressBar#breakProgressBar {
            background-color: rgba(51, 65, 85, 0.5);
            border-radius: 3px;
            border: none;
        }
        QProgressBar#breakProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #38bdf8, stop:1 #06b6d4);
            border-radius: 3px;
        }
        QLabel#tipLabel {
            font-size: 17px;
            font-weight: 500;
            color: #cbd5e1;
            line-height: 1.5;
            padding: 6px 16px;
        }
        QPushButton#snoozeBtn {
            background-color: rgba(51, 65, 85, 0.6);
            color: #e2e8f0;
            border: 1px solid rgba(148, 163, 184, 0.25);
            border-radius: 12px;
            padding: 10px 24px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#snoozeBtn:hover {
            background-color: rgba(71, 85, 105, 0.85);
            border-color: rgba(148, 163, 184, 0.5);
            color: #ffffff;
        }
        QPushButton#skipBtn {
            background-color: rgba(225, 29, 72, 0.22);
            color: #fda4af;
            border: 1px solid rgba(244, 63, 94, 0.4);
            border-radius: 12px;
            padding: 10px 24px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#skipBtn:hover {
            background-color: rgba(225, 29, 72, 0.45);
            border-color: rgba(244, 63, 94, 0.7);
            color: #ffffff;
        }
    )");
}

void OverlayWindow::prepareBreak(bool isLong, int totalDurationSec, const ExerciseGuide& guide) {
    m_isLong = isLong;
    m_totalDuration = totalDurationSec;
    m_remainingSec = totalDurationSec;

    const bool exercisesEnabled = Settings::instance().interactiveExercisesEnabled();
    const bool enableVisual = exercisesEnabled && !isLong && (guide.visualType != ExerciseVisualType::None);

    if (enableVisual) {
        m_badgeLabel->setText(!guide.badge.isEmpty() ? guide.badge : Localization::instance().badgeShortBreak());
        m_titleLabel->setText(guide.title);
        m_titleLabel->setVisible(!guide.title.isEmpty());
        m_tipLabel->setText(guide.instruction);

        m_exerciseWidget->setVisualType(guide.visualType);
        m_exerciseWidget->setVisible(true);
        m_exerciseWidget->startAnimation();
    } else {
        m_exerciseWidget->setVisualType(ExerciseVisualType::None);
        m_exerciseWidget->stopAnimation();
        m_exerciseWidget->setVisible(false);

        if (!isLong) {
            m_badgeLabel->setText(Localization::instance().badgeShortBreak());
            m_titleLabel->clear();
            m_titleLabel->setVisible(false);

            if (guide.visualType == ExerciseVisualType::None && !guide.instruction.isEmpty()) {
                m_tipLabel->setText(guide.instruction);
            } else {
                m_tipLabel->setText(Localization::instance().getRandomTip(false));
            }
        } else {
            m_badgeLabel->setText(!guide.badge.isEmpty() ? guide.badge : Localization::instance().badgeLongBreak());
            m_titleLabel->setText(guide.title);
            m_titleLabel->setVisible(!guide.title.isEmpty());
            m_tipLabel->setText(!guide.instruction.isEmpty() ? guide.instruction : Localization::instance().getRandomTip(true));
        }
    }

    int snoozeMin = std::max(1, Settings::instance().snoozeDurationSec() / 60);
    m_snoozeBtn->setText(Localization::instance().buttonSnooze(snoozeMin));
    m_skipBtn->setText(Localization::instance().buttonSkip());

    // Use 1000x scale so QPropertyAnimation interpolates at sub-second resolution
    const int maxVal = totalDurationSec * 1000;

    m_progressBar->setMaximum(maxVal);
    m_progressBar->setValue(maxVal);
    m_countdownLabel->setText(QString::number(totalDurationSec));

    if (!m_progressAnim) {
        m_progressAnim = new QPropertyAnimation(m_progressBar, "value", this);
        m_progressAnim->setEasingCurve(QEasingCurve::InOutQuad);
    }
    m_progressAnim->stop();
}

void OverlayWindow::updateCountdown(int remainingSec, int totalSec) {
    m_remainingSec = remainingSec;
    m_countdownLabel->setText(QString::number(std::max(0, remainingSec)));

    if (!m_progressAnim) return;

    // Animate from current bar position to the new target over 950ms
    // (slightly less than 1s so we don't overshoot the next tick)
    const int targetVal = std::max(0, remainingSec) * 1000;
    m_progressAnim->stop();
    m_progressAnim->setDuration(950);
    m_progressAnim->setStartValue(m_progressBar->value());
    m_progressAnim->setEndValue(targetVal);
    m_progressAnim->start();
}


void OverlayWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    // Dark semi-transparent backdrop #0c0c12 (~94% alpha = 240)
    painter.fillRect(rect(), QColor(12, 12, 18, 240));
}

void OverlayWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && !event->isAutoRepeat()) {
        if (m_skipBtn && m_skipBtn->isEnabled()) {
            m_skipBtn->setEnabled(false);
            if (m_snoozeBtn) m_snoozeBtn->setEnabled(false);
            emit skipRequested();
        }
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void OverlayWindow::hideEvent(QHideEvent* event) {
    if (m_progressAnim) {
        m_progressAnim->stop();
    }
    if (m_exerciseWidget) {
        m_exerciseWidget->stopAnimation();
    }
    QWidget::hideEvent(event);
}

void OverlayWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_screen) {
        if (windowHandle() && windowHandle()->screen() != m_screen) {
            windowHandle()->setScreen(m_screen);
        }
        if (!isFullScreen()) {
            setGeometry(m_screen->geometry());
            if (windowHandle()) {
                windowHandle()->setGeometry(m_screen->geometry());
            }
        }
    }
    if (m_exerciseWidget && m_exerciseWidget->isVisible()) {
        m_exerciseWidget->startAnimation();
    }
    raise();
}
