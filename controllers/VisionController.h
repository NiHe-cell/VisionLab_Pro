#ifndef VISIONCONTROLLER_H
#define VISIONCONTROLLER_H

#include <QObject>
#include "utilities/CameraManager.h"

class VisionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
public:
    explicit VisionController(CameraManager* cam, QObject *parent = nullptr);

    QString mode() const;
    Q_INVOKABLE void setMode(QString newMode);
    Q_INVOKABLE void startCamera();
    Q_INVOKABLE void stopCamera();

    bool running() const;
    void setRunning(bool newRunning);

signals:
    void modeChanged();
    void runningChanged();

private:
    QString m_mode = "";
    CameraManager* m_camera;
    bool m_running;
};

#endif // VISIONCONTROLLER_H
