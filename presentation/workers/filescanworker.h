#ifndef FILESCANWORKER_H
#define FILESCANWORKER_H

#include <QObject>
#include <QThread>
#include <vector>
#include <string>
#include "../../application/services/filemanagerservice.h"
#include "../../application/dto/dirsconf.h"

namespace presentation::workers {

class FileScanWorker : public QObject {
    Q_OBJECT

public:
    explicit FileScanWorker(
        std::shared_ptr<application::services::FileManagerService> fileService,
        const ::application::dto::DirsConfig & dir_paths ,
        QObject* parent = nullptr
        );

public slots:
    void process();

signals:
    void finished();
    void error(const QString& error);
    void progress(int value, const QString& message);

private:
    std::shared_ptr<application::services::FileManagerService> fileService;
    ::application::dto::DirsConfig m_dir_paths ;
};

} // namespace presentation::workers


#endif // FILESCANWORKER_H
