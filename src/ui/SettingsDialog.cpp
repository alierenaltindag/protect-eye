#include "SettingsDialog.h"
#include "core/Settings.h"
#include "core/Localization.h"
#include "services/UpdateChecker.h"
#include "utils/AutostartHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QIcon>
#include <QListView>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowIcon(QIcon(QStringLiteral(":/app_icon.svg")));
    resize(560, 480);
    setMinimumSize(500, 400);

    setupUi();
    applyStyles();
    loadValues();
    retranslateUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(18, 16, 18, 14);

    // Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);
    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(QStringLiteral(":/app_icon.svg")).pixmap(30, 30));
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 700; color: #f8fafc;"));
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setCursor(Qt::PointingHandCursor);

    auto createScrollPage = [this](QWidget* content) -> QScrollArea* {
        auto* scroll = new QScrollArea(m_tabWidget);
        scroll->setWidget(content);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->viewport()->setAutoFillBackground(false);
        scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        return scroll;
    };

    auto createDurationRow = [](QWidget* parent, QSpinBox* spin, QLabel*& unitLabel) -> QWidget* {
        auto* container = new QWidget(parent);
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

    // ==========================================
    // Tab 1: Break Schedules
    // ==========================================
    auto* schedContent = new QWidget();
    schedContent->setObjectName(QStringLiteral("schedContent"));
    auto* schedLayout = new QVBoxLayout(schedContent);
    schedLayout->setContentsMargins(4, 4, 4, 4);
    schedLayout->setSpacing(8);

    m_timesGroup = new QGroupBox(schedContent);
    auto* timesForm = new QFormLayout(m_timesGroup);
    timesForm->setVerticalSpacing(8);
    timesForm->setHorizontalSpacing(14);
    timesForm->setLabelAlignment(Qt::AlignRight);
    timesForm->setContentsMargins(14, 16, 14, 14);

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

    timesForm->addRow(m_shortIntervalLabel, createDurationRow(m_timesGroup, m_shortIntervalSpin, m_shortIntervalUnit));
    timesForm->addRow(m_shortDurationLabel, createDurationRow(m_timesGroup, m_shortDurationSpin, m_shortDurationUnit));
    timesForm->addRow(m_longIntervalLabel, createDurationRow(m_timesGroup, m_longIntervalSpin, m_longIntervalUnit));
    timesForm->addRow(m_longDurationLabel, createDurationRow(m_timesGroup, m_longDurationSpin, m_longDurationUnit));
    timesForm->addRow(m_snoozeLabel, createDurationRow(m_timesGroup, m_snoozeSpin, m_snoozeUnit));
    timesForm->addRow(m_preWarnLabel, createDurationRow(m_timesGroup, m_preWarnSpin, m_preWarnUnit));

    schedLayout->addWidget(m_timesGroup);
    schedLayout->addStretch();

    m_tabWidget->addTab(createScrollPage(schedContent), QString());

    // ==========================================
    // Tab 2: Smart Features
    // ==========================================
    auto* featContent = new QWidget();
    featContent->setObjectName(QStringLiteral("featContent"));
    auto* featContentLayout = new QVBoxLayout(featContent);
    featContentLayout->setContentsMargins(4, 4, 4, 4);
    featContentLayout->setSpacing(10);

    m_featGroup = new QGroupBox(featContent);
    auto* featLayout = new QVBoxLayout(m_featGroup);
    featLayout->setSpacing(10);
    featLayout->setContentsMargins(14, 16, 14, 14);

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

    featContentLayout->addWidget(m_featGroup);
    featContentLayout->addStretch();

    m_tabWidget->addTab(createScrollPage(featContent), QString());

    // ==========================================
    // Tab 3: Language & Updates
    // ==========================================
    auto* genContent = new QWidget();
    genContent->setObjectName(QStringLiteral("genContent"));
    auto* genContentLayout = new QVBoxLayout(genContent);
    genContentLayout->setContentsMargins(4, 4, 4, 4);
    genContentLayout->setSpacing(10);

    // Language Group
    m_langGroup = new QGroupBox(genContent);
    auto* langLayout = new QHBoxLayout(m_langGroup);
    langLayout->setContentsMargins(14, 16, 14, 14);
    langLayout->setSpacing(12);

    m_langLabel = new QLabel(m_langGroup);
    m_langCombo = new QComboBox(m_langGroup);
    m_langCombo->setCursor(Qt::PointingHandCursor);
    m_langCombo->setMinimumWidth(220);

    auto* langListView = new QListView(m_langCombo);
    langListView->setObjectName(QStringLiteral("langComboView"));
    m_langCombo->setView(langListView);

    m_langCombo->addItem(Localization::instance().langAuto(), QStringLiteral("auto"));
    for (const auto& lang : Localization::instance().availableLanguages()) {
        m_langCombo->addItem(lang.name, lang.code);
    }

    langLayout->addWidget(m_langLabel);
    langLayout->addWidget(m_langCombo);
    langLayout->addStretch();
    genContentLayout->addWidget(m_langGroup);

    // Updates Group
    m_updateGroup = new QGroupBox(genContent);
    auto* updLayout = new QVBoxLayout(m_updateGroup);
    updLayout->setContentsMargins(14, 16, 14, 14);
    updLayout->setSpacing(10);

    m_checkUpdatesCheck = new QCheckBox(m_updateGroup);
    m_checkUpdatesCheck->setCursor(Qt::PointingHandCursor);

    auto* updateRowLayout = new QHBoxLayout();
    updateRowLayout->setSpacing(10);
    updateRowLayout->setContentsMargins(0, 0, 0, 0);

    m_checkUpdatesBtn = new QPushButton(m_updateGroup);
    m_checkUpdatesBtn->setCursor(Qt::PointingHandCursor);
    m_checkUpdatesBtn->setMinimumHeight(32);

    m_updateStatusLabel = new QLabel(m_updateGroup);
    m_updateStatusLabel->setStyleSheet(QStringLiteral("color: #94a3b8; font-size: 11px;"));

    updateRowLayout->addWidget(m_checkUpdatesBtn);
    updateRowLayout->addWidget(m_updateStatusLabel, 1);

    updLayout->addWidget(m_checkUpdatesCheck);
    updLayout->addLayout(updateRowLayout);
    genContentLayout->addWidget(m_updateGroup);

    // Info footer
    auto* infoCard = new QWidget(genContent);
    auto* infoLayout = new QHBoxLayout(infoCard);
    infoLayout->setContentsMargins(4, 2, 4, 2);
    infoLayout->setSpacing(8);

    auto* verLabel = new QLabel(QStringLiteral("ProtectEye v") + UpdateChecker::currentVersion() + QStringLiteral(" • Open Source"), infoCard);
    verLabel->setStyleSheet(QStringLiteral("color: #64748b; font-size: 11px; font-weight: 500;"));
    infoLayout->addWidget(verLabel);
    infoLayout->addStretch();
    genContentLayout->addWidget(infoCard);

    genContentLayout->addStretch();

    m_tabWidget->addTab(createScrollPage(genContent), QString());

    mainLayout->addWidget(m_tabWidget, 1);

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

    connect(m_checkUpdatesBtn, &QPushButton::clicked, this, &SettingsDialog::onCheckUpdatesClicked);
    connect(&UpdateChecker::instance(), &UpdateChecker::checkStarted, this, &SettingsDialog::onCheckStarted);
    connect(&UpdateChecker::instance(), &UpdateChecker::checkFinished, this, &SettingsDialog::onCheckFinished);
}

void SettingsDialog::retranslateUi() {
    auto& loc = Localization::instance();

    auto escapeMnemonic = [](QString str) -> QString {
        return str.replace(QStringLiteral("&"), QStringLiteral("&&"));
    };

    setWindowTitle(loc.settingsTitle());
    m_titleLabel->setText(loc.settingsHeader());

    m_tabWidget->setTabText(0, QStringLiteral("⏱️  ") + escapeMnemonic(loc.tabSchedules()));
    m_tabWidget->setTabText(1, QStringLiteral("⚡  ") + escapeMnemonic(loc.tabSmartFeatures()));
    m_tabWidget->setTabText(2, QStringLiteral("🌐  ") + escapeMnemonic(loc.tabGeneral()));

    m_timesGroup->setTitle(escapeMnemonic(loc.groupBreakSchedules()));
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

    m_featGroup->setTitle(escapeMnemonic(loc.groupSmartFeatures()));
    m_soundCheck->setText(loc.checkSound());
    m_dndCheck->setText(loc.checkDnd());
    m_lockCheck->setText(loc.checkLock());
    m_autostartCheck->setText(loc.checkAutostart());
    m_interactiveExercisesCheck->setText(loc.checkInteractiveExercises());

    m_langGroup->setTitle(escapeMnemonic(loc.groupLanguage()));
    m_langLabel->setText(loc.labelLanguage());
    m_langCombo->setItemText(0, loc.langAuto());

    m_updateGroup->setTitle(escapeMnemonic(loc.groupUpdates()));
    m_checkUpdatesCheck->setText(loc.checkUpdates());
    m_checkUpdatesBtn->setText(loc.btnCheckUpdates());

    m_resetBtn->setText(loc.buttonDefaults());
    m_cancelBtn->setText(loc.buttonCancel());
    m_saveBtn->setText(loc.buttonSave());

    applyStyles();
}

void SettingsDialog::applyStyles() {
    bool isRtl = isRightToLeft();
    QString groupTitleSide = isRtl ? QStringLiteral("top right") : QStringLiteral("top left");
    QString groupTitleOffset = isRtl ? QStringLiteral("right: 14px;") : QStringLiteral("left: 14px;");
    QString dropDownSide = isRtl ? QStringLiteral("top left") : QStringLiteral("top right");
    QString dropDownBorder = isRtl ? QStringLiteral("border-right: 1px solid #475569;") : QStringLiteral("border-left: 1px solid #475569;");
    QString spinPadding = isRtl ? QStringLiteral("padding: 5px 8px 5px 24px;") : QStringLiteral("padding: 5px 24px 5px 8px;");
    QString spinUpPos = isRtl ? QStringLiteral("top left") : QStringLiteral("top right");
    QString spinDownPos = isRtl ? QStringLiteral("bottom left") : QStringLiteral("bottom right");
    QString spinRadiusUp = isRtl ? QStringLiteral("border-top-left-radius: 7px;") : QStringLiteral("border-top-right-radius: 7px;");
    QString spinRadiusDown = isRtl ? QStringLiteral("border-bottom-left-radius: 7px;") : QStringLiteral("border-bottom-right-radius: 7px;");
    QString tabMargin = isRtl ? QStringLiteral("margin-left: 6px;") : QStringLiteral("margin-right: 6px;");

    setStyleSheet(QString(R"(
        QDialog {
            background-color: #0f172a;
        }

        /* Tabs & TabBar */
        QTabWidget::pane {
            border: 1px solid #334155;
            background-color: #131d2e;
            border-radius: 12px;
            padding: 4px;
        }
        QScrollArea {
            background-color: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background-color: transparent;
            border: none;
        }
        QWidget#schedContent, QWidget#featContent, QWidget#genContent {
            background-color: transparent;
        }
        QTabBar {
            qproperty-drawBase: 0;
        }
        QTabBar::tab {
            background-color: #1e293b;
            color: #94a3b8;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 7px 16px;
            %10
            margin-bottom: 8px;
            font-size: 13px;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background-color: #0284c7;
            color: #ffffff;
            border-color: #38bdf8;
        }
        QTabBar::tab:hover:!selected {
            background-color: #273549;
            color: #f1f5f9;
        }

        /* GroupBox within Tabs */
        QGroupBox {
            font-size: 13px;
            font-weight: bold;
            color: #38bdf8;
            border: 1px solid #334155;
            border-radius: 10px;
            background-color: rgba(30, 41, 59, 0.65);
            margin-top: 10px;
            padding-top: 14px;
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

        /* ComboBox */
        QComboBox {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            font-weight: 500;
        }
        QComboBox:hover {
            border: 1px solid #64748b;
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
        QComboBox::down-arrow {
            image: url(:/spin_down.svg);
            width: 10px;
            height: 7px;
        }
        QComboBox QAbstractItemView,
        QComboBox QListView {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #334155;
            border-radius: 8px;
            selection-background-color: #0284c7;
            selection-color: #ffffff;
            outline: none;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item,
        QComboBox QListView::item {
            min-height: 28px;
            padding: 4px 10px;
            color: #f8fafc;
            background-color: #1e293b;
            border-radius: 4px;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QListView::item:hover {
            background-color: #334155;
            color: #38bdf8;
        }
        QComboBox QAbstractItemView::item:selected,
        QComboBox QListView::item:selected {
            background-color: #0284c7;
            color: #ffffff;
        }

        /* QSpinBox */
        QSpinBox {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 8px;
            %5
            font-size: 13px;
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

        /* CheckBox */
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
            width: 20px;
            height: 20px;
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
            padding: 7px 16px;
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

        /* ScrollBar */
        QScrollBar:vertical {
            background-color: #0f172a;
            width: 6px;
            margin: 0px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background-color: #334155;
            min-height: 24px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #475569;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )").arg(groupTitleSide, groupTitleOffset, dropDownSide, dropDownBorder, spinPadding, spinUpPos, spinRadiusUp, spinDownPos, spinRadiusDown, tabMargin));

    if (m_langCombo && m_langCombo->view()) {
        QPalette viewPalette = m_langCombo->view()->palette();
        viewPalette.setColor(QPalette::Base, QColor(0x1e, 0x29, 0x3b));
        viewPalette.setColor(QPalette::Window, QColor(0x1e, 0x29, 0x3b));
        viewPalette.setColor(QPalette::Text, QColor(0xf8, 0xfa, 0xfc));
        viewPalette.setColor(QPalette::Highlight, QColor(0x02, 0x84, 0xc7));
        viewPalette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        m_langCombo->view()->setPalette(viewPalette);

        if (m_langCombo->view()->parentWidget()) {
            m_langCombo->view()->parentWidget()->setPalette(viewPalette);
            m_langCombo->view()->parentWidget()->setStyleSheet(QStringLiteral(
                "background-color: #1e293b; border: 1px solid #334155; border-radius: 8px;"
            ));
        }

        m_langCombo->view()->setStyleSheet(QStringLiteral(
            "QListView {"
            "    background-color: #1e293b;"
            "    color: #f8fafc;"
            "    border: 1px solid #334155;"
            "    border-radius: 8px;"
            "    padding: 4px;"
            "    outline: none;"
            "}"
            "QListView::item {"
            "    min-height: 28px;"
            "    padding: 4px 10px;"
            "    color: #f8fafc;"
            "    background-color: #1e293b;"
            "    border-radius: 4px;"
            "}"
            "QListView::item:hover {"
            "    background-color: #334155;"
            "    color: #38bdf8;"
            "}"
            "QListView::item:selected {"
            "    background-color: #0284c7;"
            "    color: #ffffff;"
            "}"
        ));
    }
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
    m_checkUpdatesCheck->setChecked(s.checkUpdatesEnabled());

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
    s.setCheckUpdatesEnabled(m_checkUpdatesCheck->isChecked());

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

void SettingsDialog::onCheckUpdatesClicked() {
    UpdateChecker::instance().checkForUpdates(true);
}

void SettingsDialog::onCheckStarted() {
    m_checkUpdatesBtn->setEnabled(false);
    m_updateStatusLabel->setStyleSheet(QStringLiteral("color: #94a3b8; font-size: 11px;"));
    m_updateStatusLabel->setText(Localization::instance().updateStatusChecking());
}

void SettingsDialog::onCheckFinished(bool updateFound, const QString& latestVersion, const QString& errorString) {
    m_checkUpdatesBtn->setEnabled(true);
    if (!errorString.isEmpty()) {
        m_updateStatusLabel->setStyleSheet(QStringLiteral("color: #f87171; font-size: 11px;"));
        m_updateStatusLabel->setText(Localization::instance().updateStatusFailed());
    } else if (updateFound) {
        m_updateStatusLabel->setStyleSheet(QStringLiteral("color: #4ade80; font-size: 11px; font-weight: bold;"));
        m_updateStatusLabel->setText(Localization::instance().updateStatusAvailable().arg(latestVersion));
    } else {
        m_updateStatusLabel->setStyleSheet(QStringLiteral("color: #38bdf8; font-size: 11px;"));
        m_updateStatusLabel->setText(Localization::instance().updateStatusUpToDate().arg(UpdateChecker::currentVersion()));
    }
}
