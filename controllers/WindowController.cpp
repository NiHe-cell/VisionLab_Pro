/*
 * Author - Muhammed Suwaneh
*/

#include "WindowController.h"

WindowController::WindowController(QWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

void WindowController::minimize() { this->m_window->showMinimized(); }

void WindowController::close() { this->m_window->close(); }

void WindowController::maximize()
{
    if (this->m_window->visibility() == QWindow::Maximized)
        this->m_window->showNormal();
    else
        this->m_window->showMaximized();
}

void WindowController::startDrag(int mouseX, int mouseY)
{
    this->m_dragPosition = QPoint(mouseX, mouseY);
    this->m_window->startSystemMove();
}
