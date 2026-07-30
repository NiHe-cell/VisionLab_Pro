#ifndef WINDOWCONTROLLER_H
#define WINDOWCONTROLLER_H

#include <QObject>
#include <QWindow>
#include <QPoint>

class WindowController : public QObject
{
    Q_OBJECT
public:
    explicit WindowController(QWindow *window, QObject *parent = nullptr);

    Q_INVOKABLE void minimize();
    Q_INVOKABLE void maximize();
    Q_INVOKABLE void close();
    Q_INVOKABLE void startDrag(int mouseX, int mouseY);

private:
    QWindow *m_window;
    QPoint m_dragPosition;
};

#endif // WINDOWCONTROLLER_H
