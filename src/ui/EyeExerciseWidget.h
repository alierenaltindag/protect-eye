#pragma once

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointF>
#include "core/Localization.h"

class EyeExerciseWidget : public QWidget {
    Q_OBJECT

public:
    explicit EyeExerciseWidget(QWidget* parent = nullptr);
    ~EyeExerciseWidget() override;

    void setVisualType(ExerciseVisualType type);
    ExerciseVisualType visualType() const { return m_type; }

    void startAnimation();
    void stopAnimation();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawLemniscate(QPainter& painter, qint64 elapsed);
    void drawCircle(QPainter& painter, qint64 elapsed);
    void drawHorizontal(QPainter& painter, qint64 elapsed);
    void drawVertical(QPainter& painter, qint64 elapsed);
    void drawBreathing(QPainter& painter, qint64 elapsed);
    void drawDistanceFocus(QPainter& painter, qint64 elapsed);
    void drawNearFarShift(QPainter& painter, qint64 elapsed);
    void drawSqueezeBlink(QPainter& painter, qint64 elapsed);
    void drawPeripheralExpansion(QPainter& painter, qint64 elapsed);
    void drawGlowDot(QPainter& painter, const QPointF& pos, qreal radius = 8.0);

    ExerciseVisualType m_type{ExerciseVisualType::None};
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
};
