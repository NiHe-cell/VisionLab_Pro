#ifndef CAMERAIMAGEPROVIDER_H
#define CAMERAIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QMutex>
#include "CameraManager.h"

class CameraImageProvider: public QQuickImageProvider
{
public:
    CameraImageProvider(CameraManager *camera);

private:
    CameraManager* m_camera;
public:
    virtual QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

#endif // CAMERAIMAGEPROVIDER_H
