/*#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
*/

#include <QApplication>
#include <QDir>
#include <memory>

#include "domain/filesproc.h"
#include "application/services/filemanagerservice.h"
#include "infrastructure/persistence/qtfilerepository.h"
#include "presentation/widgets/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Setup logging
    infrastructure::logging::Logger::getInstance().setLogFile("afiles.log");
    infrastructure::logging::LOG_INFO("Application started");

    try {
        // Dependency injection
        auto fileRepository = std::make_unique<infrastructure::persistence::QtFileRepository>();
        auto filesProc = std::make_unique<domain::services::FilesProc>(std::move(fileRepository));
        auto fileService = std::make_shared<application::services::FileManagerService>(
            std::move(filesProc), "search_history.txt"
            );

        // Create and show main window
        presentation::widgets::MainWindow mainWindow(fileService);
        mainWindow.setWindowTitle("Qt File Manager - Clean Architecture");
        mainWindow.resize(800, 600);
        mainWindow.show();

        infrastructure::logging::LOG_INFO("Main window created successfully");

        return app.exec();

    } catch (const std::exception& e) {
        infrastructure::logging::LOG_ERROR(std::string("Application failed: ") + e.what());
        return -1;
    }
}
