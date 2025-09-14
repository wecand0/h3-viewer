#include "application.h"


Application::Application(int &argc, char **argv) : QApplication(argc, argv) {}

bool Application::eventFilter(QObject *obj, QEvent *event) {
    return QApplication::eventFilter(obj, event);
}