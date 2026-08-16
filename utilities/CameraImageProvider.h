#ifndef CAMERAIMAGEPROVIDER_H
#define CAMERAIMAGEPROVIDER_H

#include <QQuickImageProvider>

#include "CameraManager.h"

class CameraImageProvider : public QQuickImageProvider
{
public:
    explicit CameraImageProvider(CameraManager* camera);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    CameraManager* m_camera;
};

#endif // CAMERAIMAGEPROVIDER_H
