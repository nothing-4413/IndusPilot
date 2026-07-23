#include "main_window.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("IndusPilot 客户端");
    QApplication::setOrganizationName("IndusPilot");

    MainWindow window;
    window.show();
    return app.exec();
}
