/*
 * Author - Muhammed Suwaneh
*/

#include "VisionController.h"
#include <QDebug>

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

    qDebug() << newMode;

    this->m_mode = newMode;
    this->m_camera->setMode(newMode);
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
