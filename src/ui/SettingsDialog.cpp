#include "SettingsDialog.h"
#include "core/Settings.h"
#include "core/Localization.h"
#include "utils/AutostartHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QIcon>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowIcon(QIcon(QStringLiteral(":/app_icon.svg")));
    resize(520, 640);

    setupUi();
    applyStyles();
    loadValues();
    retranslateUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // Header
    auto* headerLayout = new QHBoxLayout();
    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(QStringLiteral(":/app_icon.svg")).pixmap(36, 36));
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-size: 19px; font-weight: 700; color: #f8fafc;");
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Group 0: Language & Appearance
    m_langGroup = new QGroupBox(this);
    auto* langLayout = new QHBoxLayout(m_langGroup);
    langLayout->setContentsMargins(16, 16, 16, 16);
    langLayout->setSpacing(12);

    m_langLabel = new QLabel(m_langGroup);
    m_langCombo = new QComboBox(m_langGroup);
    m_langCombo->setCursor(Qt::PointingHandCursor);
    m_langCombo->setMinimumWidth(220);

    m_langCombo->addItem(Localization::instance().langAuto(), QStringLiteral("auto"));
    for (const auto& lang : Localization::instance().availableLanguages()) {
        m_langCombo->addItem(lang.name, lang.code);
    }

    langLayout->addWidget(m_langLabel);
    langLayout->addWidget(m_langCombo);
    langLayout->addStretch();

    mainLayout->addWidget(m_langGroup);

    // Group 1: Break Times
    m_timesGroup = new QGroupBox(this);
    auto* timesForm = new QFormLayout(m_timesGroup);
    timesForm->setSpacing(12);
    timesForm->setLabelAlignment(Qt::AlignRight);

    auto createDurationRow = [this](QSpinBox* spin, QLabel*& unitLabel) -> QWidget* {
        auto* container = new QWidget(m_timesGroup);
        auto* rowLayout = new QHBoxLayout(container);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        spin->setFixedWidth(90);
        spin->setAlignment(Qt::AlignCenter);

        unitLabel = new QLabel(container);
        unitLabel->setObjectName(QStringLiteral("unitLabel"));

        rowLayout->addWidget(spin);
        rowLayout->addWidget(unitLabel);
        rowLayout->addStretch();
        return container;
    };

    m_shortIntervalSpin = new QSpinBox(m_timesGroup);
    m_shortIntervalSpin->setRange(1, 120);

    m_shortDurationSpin = new QSpinBox(m_timesGroup);
    m_shortDurationSpin->setRange(5, 300);

    m_longIntervalSpin = new QSpinBox(m_timesGroup);
    m_longIntervalSpin->setRange(10, 360);

    m_longDurationSpin = new QSpinBox(m_timesGroup);
    m_longDurationSpin->setRange(10, 600);

    m_snoozeSpin = new QSpinBox(m_timesGroup);
    m_snoozeSpin->setRange(1, 30);

    m_preWarnSpin = new QSpinBox(m_timesGroup);
    m_preWarnSpin->setRange(0, 120);

    m_shortIntervalLabel = new QLabel(m_timesGroup);
    m_shortDurationLabel = new QLabel(m_timesGroup);
    m_longIntervalLabel = new QLabel(m_timesGroup);
    m_longDurationLabel = new QLabel(m_timesGroup);
    m_snoozeLabel = new QLabel(m_timesGroup);
    m_preWarnLabel = new QLabel(m_timesGroup);

    timesForm->addRow(m_shortIntervalLabel, createDurationRow(m_shortIntervalSpin, m_shortIntervalUnit));
    timesForm->addRow(m_shortDurationLabel, createDurationRow(m_shortDurationSpin, m_shortDurationUnit));
    timesForm->addRow(m_longIntervalLabel, createDurationRow(m_longIntervalSpin, m_longIntervalUnit));
    timesForm->addRow(m_longDurationLabel, createDurationRow(m_longDurationSpin, m_longDurationUnit));
    timesForm->addRow(m_snoozeLabel, createDurationRow(m_snoozeSpin, m_snoozeUnit));
    timesForm->addRow(m_preWarnLabel, createDurationRow(m_preWarnSpin, m_preWarnUnit));

    mainLayout->addWidget(m_timesGroup);

    // Group 2: Features
    m_featGroup = new QGroupBox(this);
    auto* featLayout = new QVBoxLayout(m_featGroup);
    featLayout->setSpacing(12);
    featLayout->setContentsMargins(16, 18, 16, 16);

    m_soundCheck = new QCheckBox(m_featGroup);
    m_dndCheck = new QCheckBox(m_featGroup);
    m_lockCheck = new QCheckBox(m_featGroup);
    m_autostartCheck = new QCheckBox(m_featGroup);
    m_interactiveExercisesCheck = new QCheckBox(m_featGroup);

    m_soundCheck->setCursor(Qt::PointingHandCursor);
    m_dndCheck->setCursor(Qt::PointingHandCursor);
    m_lockCheck->setCursor(Qt::PointingHandCursor);
    m_autostartCheck->setCursor(Qt::PointingHandCursor);
    m_interactiveExercisesCheck->setCursor(Qt::PointingHandCursor);

    featLayout->addWidget(m_soundCheck);
    featLayout->addWidget(m_dndCheck);
    featLayout->addWidget(m_lockCheck);
    featLayout->addWidget(m_autostartCheck);
    featLayout->addWidget(m_interactiveExercisesCheck);

    mainLayout->addWidget(m_featGroup);

    // Action Buttons
    auto* btnLayout = new QHBoxLayout();
    m_resetBtn = new QPushButton(this);
    m_cancelBtn = new QPushButton(this);
    m_saveBtn = new QPushButton(this);
    m_saveBtn->setDefault(true);

    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addWidget(m_resetBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onLanguageChanged);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
}

void SettingsDialog::retranslateUi() {
    auto& loc = Localization::instance();

    setWindowTitle(loc.settingsTitle());
    m_titleLabel->setText(loc.settingsHeader());

    m_langGroup->setTitle(loc.groupLanguage());
    m_langLabel->setText(loc.labelLanguage());
    m_langCombo->setItemText(0, loc.langAuto());

    m_timesGroup->setTitle(loc.groupBreakSchedules());
    m_shortIntervalLabel->setText(loc.labelShortInterval());
    m_shortDurationLabel->setText(loc.labelShortDuration());
    m_longIntervalLabel->setText(loc.labelLongInterval());
    m_longDurationLabel->setText(loc.labelLongDuration());
    m_snoozeLabel->setText(loc.labelSnoozeDuration());
    m_preWarnLabel->setText(loc.labelPreWarn());

    m_shortIntervalUnit->setText(loc.unitMinutes());
    m_shortDurationUnit->setText(loc.unitSeconds());
    m_longIntervalUnit->setText(loc.unitMinutes());
    m_longDurationUnit->setText(loc.unitSeconds());
    m_snoozeUnit->setText(loc.unitMinutes());
    m_preWarnUnit->setText(loc.unitSeconds());

    m_featGroup->setTitle(loc.groupSmartFeatures());
    m_soundCheck->setText(loc.checkSound());
    m_dndCheck->setText(loc.checkDnd());
    m_lockCheck->setText(loc.checkLock());
    m_autostartCheck->setText(loc.checkAutostart());
    m_interactiveExercisesCheck->setText(loc.checkInteractiveExercises());

    m_resetBtn->setText(loc.buttonDefaults());
    m_cancelBtn->setText(loc.buttonCancel());
    m_saveBtn->setText(loc.buttonSave());

    applyStyles();
}

void SettingsDialog::applyStyles() {
    bool isRtl = isRightToLeft();
    QString groupTitleSide = isRtl ? QStringLiteral("top right") : QStringLiteral("top left");
    QString groupTitleOffset = isRtl ? QStringLiteral("right: 16px;") : QStringLiteral("left: 16px;");
    QString dropDownSide = isRtl ? QStringLiteral("top left") : QStringLiteral("top right");
    QString dropDownBorder = isRtl ? QStringLiteral("border-right: 1px solid #475569;") : QStringLiteral("border-left: 1px solid #475569;");
    QString spinPadding = isRtl ? QStringLiteral("padding: 6px 8px 6px 26px;") : QStringLiteral("padding: 6px 26px 6px 8px;");
    QString spinUpPos = isRtl ? QStringLiteral("top left") : QStringLiteral("top right");
    QString spinDownPos = isRtl ? QStringLiteral("bottom left") : QStringLiteral("bottom right");
    QString spinRadiusUp = isRtl ? QStringLiteral("border-top-left-radius: 7px;") : QStringLiteral("border-top-right-radius: 7px;");
    QString spinRadiusDown = isRtl ? QStringLiteral("border-bottom-left-radius: 7px;") : QStringLiteral("border-bottom-right-radius: 7px;");

    setStyleSheet(QString(R"(
        QDialog {
            background-color: #0f172a;
        }
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            color: #38bdf8;
            border: 1px solid #334155;
            border-radius: 12px;
            margin-top: 14px;
            padding-top: 16px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: %1;
            %2
            padding: 0 6px;
        }
        QLabel {
            color: #cbd5e1;
            font-size: 13px;
        }
        QLabel#unitLabel {
            color: #94a3b8;
            font-size: 13px;
            font-weight: 500;
        }
        
        QComboBox {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            font-weight: 500;
        }
        QComboBox:focus {
            border: 1px solid #38bdf8;
            background-color: #243047;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: %3;
            width: 26px;
            %4
        }
        QComboBox QAbstractItemView {
            background-color: #0f172a;
            color: #f8fafc;
            border: 1px solid #334155;
            selection-background-color: #1e293b;
            selection-color: #38bdf8;
            padding: 6px;
        }

        /* Modern Clean Number Input (QSpinBox) */
        QSpinBox {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 8px;
            %5
            font-size: 14px;
            font-weight: 600;
        }
        QSpinBox:focus {
            border: 1px solid #38bdf8;
            background-color: #243047;
        }
        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: %6;
            width: 22px;
            %7
            background-color: #334155;
            %4
            border-bottom: 1px solid #334155;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: %8;
            width: 22px;
            %9
            background-color: #334155;
            %4
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #0284c7;
        }
        QSpinBox::up-arrow {
            image: url(:/spin_up.svg);
            width: 10px;
            height: 7px;
        }
        QSpinBox::down-arrow {
            image: url(:/spin_down.svg);
            width: 10px;
            height: 7px;
        }
        
        /* Modern Vibrant CheckBox Styling */
        QCheckBox {
            color: #e2e8f0;
            font-size: 13px;
            font-weight: 500;
            spacing: 12px;
            padding: 4px 6px;
            border-radius: 6px;
        }
        QCheckBox:hover {
            color: #ffffff;
            background-color: rgba(56, 189, 248, 0.06);
        }
        QCheckBox::indicator {
            width: 22px;
            height: 22px;
        }
        QCheckBox::indicator:unchecked {
            image: url(:/checkbox_unchecked.svg);
        }
        QCheckBox::indicator:unchecked:hover {
            image: url(:/checkbox_unchecked_hover.svg);
        }
        QCheckBox::indicator:checked {
            image: url(:/checkbox_checked.svg);
        }
        QCheckBox::indicator:checked:hover {
            image: url(:/checkbox_checked.svg);
        }

        /* Buttons */
        QPushButton {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #475569;
            border-radius: 8px;
            padding: 8px 18px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #334155;
            border-color: #64748b;
        }
        QPushButton#saveBtn, QPushButton:default {
            background-color: #0284c7;
            color: #ffffff;
            border: 1px solid #38bdf8;
        }
        QPushButton:default:hover {
            background-color: #0369a1;
            border-color: #7dd3fc;
        }
    )").arg(groupTitleSide, groupTitleOffset, dropDownSide, dropDownBorder, spinPadding, spinUpPos, spinRadiusUp, spinDownPos, spinRadiusDown));
}

void SettingsDialog::loadValues() {
    auto& s = Settings::instance();
    m_shortIntervalSpin->setValue(s.shortBreakIntervalSec() / 60);
    m_shortDurationSpin->setValue(s.shortBreakDurationSec());
    m_longIntervalSpin->setValue(s.longBreakIntervalSec() / 60);
    m_longDurationSpin->setValue(s.longBreakDurationSec());
    m_snoozeSpin->setValue(s.snoozeDurationSec() / 60);
    m_preWarnSpin->setValue(s.preBreakNotificationSec());

    m_soundCheck->setChecked(s.soundEnabled());
    m_dndCheck->setChecked(s.dndCheckEnabled());
    m_lockCheck->setChecked(s.screenLockCheckEnabled());
    m_autostartCheck->setChecked(AutostartHelper::isAutostartEnabled());
    m_interactiveExercisesCheck->setChecked(s.interactiveExercisesEnabled());

    QString lang = s.language().toLower();
    int foundIdx = m_langCombo->findData(lang);
    if (foundIdx >= 0) {
        m_langCombo->setCurrentIndex(foundIdx);
    } else {
        m_langCombo->setCurrentIndex(0);
    }
}

void SettingsDialog::onLanguageChanged(int index) {
    QString code = m_langCombo->itemData(index).toString();
    if (code.isEmpty()) code = QStringLiteral("auto");
    Localization::instance().setLanguageByCode(code);
    retranslateUi();
}

void SettingsDialog::onSave() {
    auto& s = Settings::instance();
    s.setShortBreakIntervalSec(m_shortIntervalSpin->value() * 60);
    s.setShortBreakDurationSec(m_shortDurationSpin->value());
    s.setLongBreakIntervalSec(m_longIntervalSpin->value() * 60);
    s.setLongBreakDurationSec(m_longDurationSpin->value());
    s.setSnoozeDurationSec(m_snoozeSpin->value() * 60);
    s.setPreBreakNotificationSec(m_preWarnSpin->value());

    s.setSoundEnabled(m_soundCheck->isChecked());
    s.setDndCheckEnabled(m_dndCheck->isChecked());
    s.setScreenLockCheckEnabled(m_lockCheck->isChecked());
    s.setAutostartEnabled(m_autostartCheck->isChecked());
    s.setInteractiveExercisesEnabled(m_interactiveExercisesCheck->isChecked());

    QString langCode = m_langCombo->currentData().toString();
    if (langCode.isEmpty()) langCode = QStringLiteral("auto");
    s.setLanguage(langCode);

    AutostartHelper::setAutostartEnabled(m_autostartCheck->isChecked());

    s.save();
    accept();
}

void SettingsDialog::onResetDefaults() {
    Settings::instance().resetToDefaults();
    AutostartHelper::setAutostartEnabled(true);
    loadValues();
    retranslateUi();
}
