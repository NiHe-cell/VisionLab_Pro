#ifndef WRONGIIDPLUGIN_H
#define WRONGIIDPLUGIN_H

#include <QObject>

class WrongIidPlugin : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.visionlab.wrong/1.0")
};

#endif // WRONGIIDPLUGIN_H
