/*
 * Author - Muhammed Suwaneh
*/

#include "CameraImageProvider.h"

CameraImageProvider::CameraImageProvider(CameraManager *camera) : QQuickImageProvider(QQuickImageProvider::Image), m_camera(camera) {}

QImage CameraImageProvider::requestImage(
    const QString &,
    QSize *size,
    const QSize &requestedSize)
{
    QImage img = this->m_camera->frame();

    if (img.isNull()) {
        return QImage(); // QML will retry
    }

    if (size)
        *size = img.size();

    if (requestedSize.isValid())
        return img.scaled(requestedSize, Qt::KeepAspectRatio);

    return img;
}
