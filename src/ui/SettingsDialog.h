#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onSave();
    void onResetDefaults();
    void onLanguageChanged(int index);
    void onCheckUpdatesClicked();
    void onCheckStarted();
    void onCheckFinished(bool updateFound, const QString& latestVersion, const QString& errorString);

private:
    void setupUi();
    void loadValues();
    void applyStyles();
    void retranslateUi();

    // Language
    QGroupBox* m_langGroup{nullptr};
    QLabel* m_langLabel{nullptr};
    QComboBox* m_langCombo{nullptr};

    // Header & Groups
    QLabel* m_titleLabel{nullptr};
    QGroupBox* m_timesGroup{nullptr};
    QGroupBox* m_featGroup{nullptr};

    // Form labels
    QLabel* m_shortIntervalLabel{nullptr};
    QLabel* m_shortDurationLabel{nullptr};
    QLabel* m_longIntervalLabel{nullptr};
    QLabel* m_longDurationLabel{nullptr};
    QLabel* m_snoozeLabel{nullptr};
    QLabel* m_preWarnLabel{nullptr};

    // Unit labels
    QLabel* m_shortIntervalUnit{nullptr};
    QLabel* m_shortDurationUnit{nullptr};
    QLabel* m_longIntervalUnit{nullptr};
    QLabel* m_longDurationUnit{nullptr};
    QLabel* m_snoozeUnit{nullptr};
    QLabel* m_preWarnUnit{nullptr};

    // Inputs
    QSpinBox* m_shortIntervalSpin{nullptr};
    QSpinBox* m_shortDurationSpin{nullptr};
    QSpinBox* m_longIntervalSpin{nullptr};
    QSpinBox* m_longDurationSpin{nullptr};
    QSpinBox* m_snoozeSpin{nullptr};
    QSpinBox* m_preWarnSpin{nullptr};

    QCheckBox* m_soundCheck{nullptr};
    QCheckBox* m_dndCheck{nullptr};
    QCheckBox* m_lockCheck{nullptr};
    QCheckBox* m_autostartCheck{nullptr};
    QCheckBox* m_interactiveExercisesCheck{nullptr};
    QCheckBox* m_checkUpdatesCheck{nullptr};

    QPushButton* m_checkUpdatesBtn{nullptr};
    QLabel* m_updateStatusLabel{nullptr};

    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QPushButton* m_resetBtn{nullptr};
};

