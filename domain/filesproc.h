
// SERVICE

#ifndef FILESPROC_H
#define FILESPROC_H

#include "filenode.h"
#include "filerepository.h"
#include "../application/dto/dirsconf.h"
#include <memory>
#include <vector>
#include <QObject>
#include "../application/dto/scanprogress.h"
#include <QElapsedTimer>

namespace domain::services {

class FilesProc : public QObject {
    Q_OBJECT

signals:
    // для прогресса сканирования
    void scanProgress(int current, const QString& currentItem);

    // Сигналы завершения операций
    //void scanCompleted(const std::vector<entities::FileNode>& fileSystem);
    //void operationFailed(const QString& error);

public:
    explicit FilesProc(std::unique_ptr<repositories::IFileRepository> repository);

    // Удаляем копирование (т.к. unique_ptr нельзя копировать)
    FilesProc(const FilesProc&) = delete;
    FilesProc& operator=(const FilesProc&) = delete;
    // Разрешаем перемещение
    FilesProc(FilesProc&&) = default;
    FilesProc& operator=(FilesProc&&) = default;
    //

    // Асинхронные операции
    //void startScan(const application::dto::DirsConfig& config);
    //void cancel();
    //

    std::vector<entities::FileNode> scanFileSystem(const ::application::dto::DirsConfig dirs_paths);
    std::vector<int> searchFiles(const std::vector<entities::FileNode>& fileSystem,
        const QString & searchTerm
        );
    void stopScanFiles() { stop_scan = true ; }
    void resetStopState() { stop_scan = false ; }

    //const std::vector<entities::FileNode>& getFileSystem() const { return fileSystem; }

private:
    // Внутренний контекст сканирования
    struct ScanContext {
        // Ссылки на данные сканирования
        std::vector<entities::FileNode>& fileSystem;
        const ::application::dto::DirsConfig& dirs_config;

        // Ссылка на флаг остановки (управляется извне)
        //std::atomic<bool>& stopFlag;

        // Счетчики прогресса
        int& totalScanned;
        QElapsedTimer& progressTimer;

        // Ссылка на родителя для эмита сигналов
        //FilesProc* parent;

        // Вспомогательные методы
        bool shouldExclude(const QString& path) const;
        void emitProgress(const QString& currentPath);
    };

    void scanDirectoryRecursive(ScanContext& context,
                                const QString& path,
                                int parentIndex);


    // void scanDirectoryRecursive(std::vector<entities::FileNode>& fileSystem, // Принимает для заполнения
    //     const QString &path,
    //     int parentIndex );
    domain::entities::FileType qFileInfoToFileType(const QFileInfo& fileInfo) const ;

    std::atomic<bool> stop_scan{ false } ;
    //size_t totalScanned{ 0 } ;
    //QElapsedTimer progressTimer;

    std::unique_ptr<repositories::IFileRepository> repository;
};

} // namespace domain::services


#endif // FILESPROC_H
