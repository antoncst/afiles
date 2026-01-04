application/dto/scanprogress.h
                        cpp

#ifndef SCANPROGRESS_H
#define SCANPROGRESS_H

#include <QString>

namespace application::dto {

    struct ScanProgress {
        int current = 0;          // Текущее количество обработанных элементов
        QString currentItem;      // Текущий файл/директория

        // Вспомогательные методы
        bool isValid() const { return current >= 0; }
        QString toString() const {
            return QString("Processed: %1 | Current: %2").arg(current).arg(currentItem);
        }
    };

} // namespace application::dto

#endif // SCANPROGRESS_H

2. Минималистичный FileScanner

            domain/services/filescanner.h
                     cpp

#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <QObject>
#include <memory>
#include <vector>
#include "../entities/filenode.h"
#include "../repositories/filerepository.h"
#include "../../application/dto/directoriesconfig.h"
#include "../../application/dto/scanprogress.h"

                 namespace domain::services {

class FileScanner : public QObject {
    Q_OBJECT

signals:
    // ТОЛЬКО ОДИН СИГНАЛ для прогресса сканирования!
    void scanProgress(int current, const QString& currentItem);

    // Сигналы завершения операций (их оставляем отдельными)
    void scanCompleted(const std::vector<entities::FileNode>& fileSystem);
    void searchCompleted(const std::vector<int>& resultIndices);
    void operationFailed(const QString& error);

public:
    explicit FileScanner(std::unique_ptr<repositories::FileRepository> repository);

    // Асинхронные операции
    void startScan(const application::dto::DirectoriesConfig& config);
    void startSearch(const std::vector<entities::FileNode>& fileSystem,
                     const std::string& searchTerm);

    void cancel();

private:
    void scanDirectoryRecursive(
        const std::string& path,
        int parentIndex,
        std::vector<entities::FileNode>& fileSystem,
        int& totalScanned,
        const application::dto::DirectoriesConfig& config
        );

    bool shouldExcludePath(
        const std::string& path,
        const application::dto::DirectoriesConfig& config
        ) const;

    std::unique_ptr<repositories::FileRepository> repository;
    std::atomic<bool> cancelled{false};
};

} // namespace domain::services

#endif // FILESCANNER_H

domain/services/filescanner.cpp
                        cpp

#include "filescanner.h"
#include <QThread>
#include <QElapsedTimer>
#include <QtConcurrent>

                    namespace domain::services {

    FileScanner::FileScanner(std::unique_ptr<repositories::FileRepository> repository)
        : QObject(nullptr), repository(std::move(repository)) {}

    void FileScanner::startScan(const application::dto::DirectoriesConfig& config) {
        cancelled = false;

        // Запускаем в отдельном потоке
        QtConcurrent::run([this, config]() {
            try {
                std::vector<entities::FileNode> fileSystem;
                int totalScanned = 0;
                QElapsedTimer progressTimer;
                progressTimer.start();

                for (const auto& rootPath : config.roots) {
                    if (cancelled) break;

                    if (!repository->exists(rootPath)) {
                        emit operationFailed(QString("Directory does not exist: %1")
                                                 .arg(QString::fromStdString(rootPath)));
                        continue;
                    }

                    if (shouldExcludePath(rootPath, config)) {
                        continue;
                    }

                    // Создаем корневой узел
                    entities::FileNode rootNode;
                    rootNode.index = static_cast<int>(fileSystem.size());
                    rootNode.parentIndex = -1;
                    rootNode.name = repository->getFileName(rootPath);
                    rootNode.type = repository->getFileType(rootPath);
                    rootNode.size = repository->getFileSize(rootPath);
                    rootNode.modifiedTime = repository->getModifiedTime(rootPath);
                    rootNode.createdTime = repository->getCreatedTime(rootPath);
                    rootNode.permissions = repository->getPermissions(rootPath);

                    fileSystem.push_back(rootNode);
                    totalScanned++;

                    // Эмитим прогресс
                    if (progressTimer.elapsed() >= 100) { // Каждые 100мс
                        emit scanProgress(totalScanned, QString::fromStdString(rootPath));
                        progressTimer.restart();
                    }

                    // Рекурсивное сканирование
                    scanDirectoryRecursive(rootPath, rootNode.index, fileSystem,
                                           totalScanned, config);

                    if (cancelled) break;
                }

                if (!cancelled) {
                    emit scanCompleted(fileSystem);
                }

            } catch (const std::exception& e) {
                emit operationFailed(QString("Scan error: %1").arg(e.what()));
            }
        });
    }

    void FileScanner::scanDirectoryRecursive(
        const std::string& path,
        int parentIndex,
        std::vector<entities::FileNode>& fileSystem,
        int& totalScanned,
        const application::dto::DirectoriesConfig& config) {

        if (cancelled) return;

        if (shouldExcludePath(path, config)) {
            return;
        }

        static QElapsedTimer emitTimer;
        if (emitTimer.isValid() && emitTimer.elapsed() < 100) {
            emitTimer.restart();
        }

        try {
            auto children = repository->scanDirectory(path, parentIndex);

            int nextIndex = static_cast<int>(fileSystem.size());
            for (auto& child : children) {
                if (cancelled) break;

                child.index = nextIndex++;
                fileSystem.push_back(child);
                totalScanned++;

                // Обновляем родителя
                if (parentIndex >= 0 && parentIndex < static_cast<int>(fileSystem.size())) {
                    fileSystem[parentIndex].childrenIndices.push_back(child.index);
                }

                // Эмитим прогресс каждые 100мс или каждые 100 элементов
                if (totalScanned % 100 == 0 ||
                    (emitTimer.isValid() && emitTimer.elapsed() >= 100)) {
                    emit scanProgress(totalScanned, QString::fromStdString(child.name));
                    if (emitTimer.isValid()) emitTimer.restart();
                }

                // Рекурсивно сканируем поддиректории
                if (child.isDirectory()) {
                    std::string childPath = child.getFullPath(fileSystem);
                    scanDirectoryRecursive(childPath, child.index, fileSystem,
                                           totalScanned, config);
                }
            }

        } catch (const std::exception& e) {
            // Пропускаем директории с ошибками доступа
            emit operationFailed(QString("Cannot scan %1: %2")
                                     .arg(QString::fromStdString(path))
                                     .arg(e.what()));
        }
    }

    void FileScanner::startSearch(
        const std::vector<entities::FileNode>& fileSystem,
        const std::string& searchTerm) {

        cancelled = false;

        QtConcurrent::run([this, fileSystem, searchTerm]() {
            try {
                std::vector<int> resultIndices;

                for (size_t i = 0; i < fileSystem.size(); ++i) {
                    if (cancelled) break;

                    const auto& node = fileSystem[i];
                    if (node.name.find(searchTerm) != std::string::npos) {
                        resultIndices.push_back(static_cast<int>(i));
                    }
                }

                if (!cancelled) {
                    emit searchCompleted(resultIndices);
                }

            } catch (const std::exception& e) {
                emit operationFailed(QString("Search error: %1").arg(e.what()));
            }
        });
    }

    bool FileScanner::shouldExcludePath(
        const std::string& path,
        const application::dto::DirectoriesConfig& config) const {

        for (const auto& excludePath : config.exclude) {
            if (path.find(excludePath) == 0) {
                return true;
            }
        }
        return false;
    }

    void FileScanner::cancel() {
        cancelled = true;
    }

} // namespace domain::services

3. Упрощенный FileManagerService

            application/services/filemanagerservice.h
                     cpp

#ifndef FILEMANAGERSERVICE_H
#define FILEMANAGERSERVICE_H

#include <QObject>
#include <memory>
#include <vector>
#include <string>
#include "../dto/directoriesconfig.h"
#include "../dto/scanprogress.h"
#include "../../domain/services/filescanner.h"

                 namespace application::services {

    class FileManagerService : public QObject {
        Q_OBJECT

    signals:
        // ТОЛЬКО ОДИН СИГНАЛ прогресса для UI!
        void scanProgressUpdated(int current, const QString& currentItem);

        // Сигналы завершения
        void scanCompleted();
        void searchCompleted(const std::vector<int>& resultIndices);
        void operationFailed(const QString& error);

    public:
        FileManagerService(
            std::unique_ptr<domain::services::FileScanner> scanner,
            const std::string& historyFile = "search_history.txt"
            );

        void startScan();
        void startSearch(const std::string& searchTerm);
        void cancelCurrentOperation();

        // Геттеры
        const std::vector<domain::entities::FileNode>& getFileSystem() const { return fileSystem; }
        const std::vector<int>& getSearchResultIndices() const { return searchResultIndices; }
        const application::dto::DirectoriesConfig& getDirectoriesConfig() const { return directoriesConfig; }

        void setDirectoriesConfig(const application::dto::DirectoriesConfig& config);

    private slots:
        void onScannerProgress(int current, const QString& currentItem);
        void onScannerScanCompleted(const std::vector<domain::entities::FileNode>& fileSystem);
        void onScannerSearchCompleted(const std::vector<int>& resultIndices);
        void onScannerOperationFailed(const QString& error);

    private:
        void saveSearchToHistory(const std::string& searchTerm);
        void loadSearchHistory();

        std::unique_ptr<domain::services::FileScanner> scanner;
        std::vector<domain::entities::FileNode> fileSystem;
        std::vector<int> searchResultIndices;
        std::vector<std::string> searchHistory;
        application::dto::DirectoriesConfig directoriesConfig;
        std::string historyFilePath;
    };

} // namespace application::services

#endif // FILEMANAGERSERVICE_H

application/services/filemanagerservice.cpp
                             cpp

#include "filemanagerservice.h"
#include <fstream>
#include <algorithm>

                         namespace application::services {

    FileManagerService::FileManagerService(
        std::unique_ptr<domain::services::FileScanner> scanner,
        const std::string& historyFile)
        : scanner(std::move(scanner)), historyFilePath(historyFile) {

        // Подключаем сигналы от сканера
        connect(this->scanner.get(), &domain::services::FileScanner::scanProgress,
                this, &FileManagerService::onScannerProgress);
        connect(this->scanner.get(), &domain::services::FileScanner::scanCompleted,
                this, &FileManagerService::onScannerScanCompleted);
        connect(this->scanner.get(), &domain::services::FileScanner::searchCompleted,
                this, &FileManagerService::onScannerSearchCompleted);
        connect(this->scanner.get(), &domain::services::FileScanner::operationFailed,
                this, &FileManagerService::onScannerOperationFailed);

        loadSearchHistory();
    }

    void FileManagerService::startScan() {
        if (directoriesConfig.roots.empty()) {
            emit operationFailed("No root directories configured");
            return;
        }

        scanner->startScan(directoriesConfig);
    }

    void FileManagerService::startSearch(const std::string& searchTerm) {
        if (searchTerm.empty()) {
            searchResultIndices.clear();
            emit searchCompleted(searchResultIndices);
            return;
        }

        saveSearchToHistory(searchTerm);
        scanner->startSearch(fileSystem, searchTerm);
    }

    void FileManagerService::onScannerProgress(int current, const QString& currentItem) {
        // Просто перенаправляем сигнал в UI
        emit scanProgressUpdated(current, currentItem);
    }

    void FileManagerService::onScannerScanCompleted(const std::vector<domain::entities::FileNode>& fileSystem) {
        this->fileSystem = fileSystem;
        searchResultIndices.clear();
        emit scanCompleted();
    }

    void FileManagerService::onScannerSearchCompleted(const std::vector<int>& resultIndices) {
        this->searchResultIndices = resultIndices;
        emit searchCompleted(resultIndices);
    }

    void FileManagerService::onScannerOperationFailed(const QString& error) {
        emit operationFailed(error);
    }

    void FileManagerService::cancelCurrentOperation() {
        scanner->cancel();
    }

    void FileManagerService::setDirectoriesConfig(const application::dto::DirectoriesConfig& config) {
        directoriesConfig = config;
    }

    void FileManagerService::saveSearchToHistory(const std::string& searchTerm) {
        searchHistory.erase(
            std::remove(searchHistory.begin(), searchHistory.end(), searchTerm),
            searchHistory.end()
            );

        searchHistory.insert(searchHistory.begin(), searchTerm);

        if (searchHistory.size() > 50) {
            searchHistory.resize(50);
        }

        std::ofstream file(historyFilePath);
        if (file.is_open()) {
            for (const auto& term : searchHistory) {
                file << term << "\n";
            }
        }
    }

    void FileManagerService::loadSearchHistory() {
        std::ifstream file(historyFilePath);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                searchHistory.push_back(line);
            }
        }
    }

} // namespace application::services

4. Минималистичный MainWindow

            presentation/widgets/mainwindow.cpp (ключевые части)
                cpp

                void MainWindow::setupConnections() {
    // Всего 4 подключения!
    connect(fileService.get(), &application::services::FileManagerService::scanProgressUpdated,
            this, &MainWindow::onScanProgressUpdated);
    connect(fileService.get(), &application::services::FileManagerService::scanCompleted,
            this, &MainWindow::onScanCompleted);
    connect(fileService.get(), &application::services::FileManagerService::searchCompleted,
            this, &MainWindow::onSearchCompleted);
    connect(fileService.get(), &application::services::FileManagerService::operationFailed,
            this, &MainWindow::onOperationFailed);
}

void MainWindow::onScanProgressUpdated(int current, const QString& currentItem) {
    // Обновляем статус бар каждые 100мс
    QString status = QString("Scanned: %1 items | Current: %2")
                         .arg(current)
                         .arg(currentItem);

    statusBar()->showMessage(status);

    // Обновляем прогресс диалог
    if (scanProgressDialog->isVisible()) {
        scanProgressDialog->setLabelText(status);
    }
}

void MainWindow::onScanClicked() {
    scanProgressDialog->setLabelText("Starting scan...");
    scanProgressDialog->setRange(0, 0); // Indeterminate
    scanProgressDialog->show();

    fileService->startScan();
}

void MainWindow::onScanCompleted() {
    scanProgressDialog->hide();

    const auto& fileSystem = fileService->getFileSystem();
    QString message = QString("Scan completed: %1 items found").arg(fileSystem.size());

    showInfo(message);
    statusBar()->showMessage(message, 3000);
}

void MainWindow::onSearchClicked() {
    QString term = searchEdit->text().trimmed();
    if (term.isEmpty()) return;

    // Для поиска не показываем прогресс-диалог, только статус бар
    statusBar()->showMessage("Searching...");

    fileService->startSearch(term.toStdString());
}

void MainWindow::onSearchCompleted(const std::vector<int>& resultIndices) {
    updateSearchResultsView();


    QString message = QString("Search completed: %1 files found").arg(resultIndices.size());
    statusBar()->showMessage(message, 3000);
}

void MainWindow::onOperationFailed(const QString& error) {
    scanProgressDialog->hide();
    showError(error);
}

