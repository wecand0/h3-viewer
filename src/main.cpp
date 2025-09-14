#include "application.h"


void sigint_callback_handler(int signum);

int main(int argc, char *argv[]) {
    signal(SIGINT, &sigint_callback_handler);
    signal(SIGILL, &sigint_callback_handler);
    signal(SIGFPE, &sigint_callback_handler);
    signal(SIGSEGV, &sigint_callback_handler);
    signal(SIGTERM, &sigint_callback_handler);
    signal(SIGABRT, &sigint_callback_handler);

    qputenv("QSG_RENDER_LOOP", "basic"); // basic //threaded

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#endif

    Application::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
    Application app(argc, argv);

    QLocale::setDefault(QLocale(QLocale::Russian));

    Application::setApplicationDisplayName("h3-viewer");
    QQuickStyle::setStyle("Universal");

    app.installEventFilter(&app);

    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} %{category} %{message}\n");


    auto window = new MainWindow();


    QObject::connect(&app, &QGuiApplication::aboutToQuit, [window]() {
        delete window;
    });

    return Application::exec();
}

void sigint_callback_handler(int signum) {
    printf("%s %d", "Приложение остановлено с кодом: ", signum);
    exit(signum);
}
