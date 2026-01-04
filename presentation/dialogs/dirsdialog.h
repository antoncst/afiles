#ifndef DIRSDIALOG_H
#define DIRSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include "../../application/dto/dirsconf.h"

namespace presentation::dialogs {

class DirectoriesDialog : public QDialog {
    Q_OBJECT

public:
    explicit DirectoriesDialog(QWidget* parent = nullptr);

    // Установка и получение конфигурации
    void setConfig(const application::dto::DirsConfig& config);
    application::dto::DirsConfig getConfig() const;

    // Статический метод для удобства
    static application::dto::DirsConfig getDirsConfig(
        const application::dto::DirsConfig& initial = {},
        QWidget* parent = nullptr);

private slots:
    void onAddRootClicked();
    void onRemoveRootClicked();
    void onAddExcludeClicked();
    void onRemoveExcludeClicked();
    void onMoveRootUp();
    void onMoveRootDown();
    void UpdateDialogState();
    void onRootDirsEditFinished() ;
    void onExclDirsEditFinished() ;

private:
    void setupUI();
    void setupConnections();
    void updateRootsList();
    void updateExcludeList();
    void updateRootsEdit();
    void updateExcludeEdit();
    void accept() override;
    bool Validate() ;
    bool showValidationWarnings(const application::dto::DirsConfig::ValidationResult& validation) ;
    QStringList vectorToStringList(const std::vector<std::string>& vec) const;
    std::vector<QString> stringListToVector(const QStringList& list) const;

    // UI Components
    QHBoxLayout * hbox_root ;
    QLabel * label_root_dirs ;
    QLineEdit * edit_root_dirs ;

    QHBoxLayout * hbox_excl ;
    QLabel * label_excl_dirs ;
    QLineEdit * edit_excl_dirs ;

    QListWidget* rootsListWidget;
    QListWidget* excludeListWidget;

    QPushButton* addRootButton;
    QPushButton* removeRootButton;
    QPushButton* moveRootUpButton;
    QPushButton* moveRootDownButton;

    QPushButton* addExcludeButton;
    QPushButton* removeExcludeButton;

    QDialogButtonBox* buttonBox;

    // Data
    application::dto::DirsConfig dirs_config;
};

} // namespace presentation::dialogs


#endif // DIRSDIALOG_H
