/*
 * Author - Muhammed Suwaneh
*/

#include "VisionController.h"

#include <QDebug>

#include "core/VisionTypes.h"

VisionController::VisionController(CameraManager* cam, QObject *parent) : QObject{parent}, m_camera(cam), m_running(false)
{}

QString VisionController::mode() const
{
    return this->m_mode;
}

void VisionController::setMode(QString newMode)
{
    if (this->m_mode == newMode)
        return;

    // QML 侧仍是展示字符串；进入领域层前映射为枚举。
    this->m_mode = newMode;
    this->m_camera->setMode(visionlab::detectionModeFromLabel(newMode.toStdString()));
    emit modeChanged();
}

void VisionController::startCamera()
{
    if (this->m_camera->start())
        setRunning(true);
}

void VisionController::stopCamera()
{
    setRunning(false);
    this->m_camera->stop();
}

bool VisionController::running() const
{
    return m_running;
}

void VisionController::setRunning(bool newRunning)
{
    if (m_running == newRunning)
        return;
    m_running = newRunning;
    emit runningChanged();
}
