#include "filescanworker.h"
//#include <QDebug>

namespace presentation::workers {

FileScanWorker::FileScanWorker(
    std::shared_ptr<application::services::FileManagerService> fileService,
    const ::application::dto::DirsConfig & dir_paths ,
    QObject* parent)
    : QObject(parent), fileService(fileService), m_dir_paths( dir_paths ) {}

void FileScanWorker::process() {
    try {
        qDebug() << "starting scan Worker" ;
        emit progress(0, "Starting file system scan...");

        // Устанавливаем корневые директории
        fileService->setDirectoriesConfig( m_dir_paths );

        // Выполняем сканирование
        fileService->scanFileSystem();

        emit progress(100, "Scan completed successfully");
        emit finished();

    } catch (const std::exception& e) {
        emit error(QString("Scan failed: %1").arg(e.what()));
    }
}

} // namespace presentation::workers
