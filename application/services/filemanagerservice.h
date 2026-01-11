#ifndef FILEMANAGERSERVICE_H
#define FILEMANAGERSERVICE_H

#include "../../domain/filenode.h"
#include "../../domain/filesproc.h"
#include "../../domain/dirspaths.h"
#include <vector>
#include <memory>
#include <string>

namespace application::services {

class FileManagerService : public QObject {
    Q_OBJECT
signals:
    // сигнал прогресса для UI!
    void scanProgressUpdated(int current, const QString& currentItem) ;

public:
    FileManagerService(std::unique_ptr<domain::services::FilesProc> filesProc,
        const std::string& historyFile
        );

    //void setRootDirectories(const ::application::dto::DirsConfig & dir_paths);
    void scanFileSystem();
    void stopScanFiles() { filesProc->stopScanFiles() ; }
    void resetStopState() { filesProc->resetStopState() ; }
    void searchFiles(const std::string& searchTerm);

    // File operations
    bool createDirectory(const std::string& path);
    bool remove(const std::string& path);
    bool rename(const std::string& oldPath, const std::string& newPath);
    bool copy(const std::string& source, const std::string& destination);

    // Getters
    const std::vector<domain::entities::FileNode>& getFileSystem() const
        { return fileSystem ; }
    const std::vector<int>& getSearchResultIndices() const
        { return searchResultIndices; }
    const std::vector<std::string>& getSearchHistory() const
        { return searchHistory; }

    // Методы для работы с конфигурацией директорий
    void setDirectoriesConfig(const application::dto::DirsConfig& config);
    const application::dto::DirsConfig getDirectoriesConfig() const;
    void setShowHidden( bool show_hidden ) {
        m_dirs_config.show_hidden = show_hidden ;
    }

    // Helper methods
    QString getNodeFullPath(int index) const;
    //const domain::entities::FileNode* getNode(int index) const;
private slots:
    void onScannerProgress(int current, const QString& currentItem) ;
private:
    void saveSearchToHistory(const std::string& searchTerm);
    void loadSearchHistory();
    ::application::dto::DirsConfig normalizeDirsForScanning(
        const ::application::dto::DirsConfig& config) const ;

    std::unique_ptr<domain::services::FilesProc> filesProc { nullptr } ;
    std::vector<domain::entities::FileNode> fileSystem; // Здесь храним дерево (в памяти)
    std::vector<int> searchResultIndices;
    std::vector<std::string> searchHistory;
    ::application::dto::DirsConfig m_dirs_config = { {} , {} , false } ;
    std::string historyFilePath;
};

} // namespace application::services

#endif // FILEMANAGERSERVICE_H
