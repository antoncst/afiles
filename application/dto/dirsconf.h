#ifndef DIRSCONF_H
#define DIRSCONF_H

#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <QString>
#include <QStringList>
#include <QDir>

namespace application::dto {

struct DirsConfig {
    std::vector<QString> roots;
    std::vector<QString> exclude; // do not search in these dirs

    bool operator==(const DirsConfig& other) const {
        return roots == other.roots && exclude == other.exclude;
    }

    bool operator!=(const DirsConfig& other) const {
        return !(*this == other);
    }
    // obsolete - больше не нужен
    // set_dirs разбирает строку sdir_semistr - in parameter , разделённую ';' на вектора строк :
    //      корневые директории для поиска и директории, исключаемые из поиска (начинаются с '!')
    // dirs_paths - in out parameter.
    // если dirs_paths изменился, возвращает true
    // если dirs_paths не изменился, возвращает false
    // если какая-либо директория не существует, выбрасывает исключение

    bool set_dirs( const QString & dir_semistr)
    {

        QStringList qsl_dirs_root ;
        QStringList qsl_dirs_exclude ;
        QStringList qsl_dirs_all = dir_semistr.split(";" , Qt::SkipEmptyParts ) ; //qt6
        //QStringList dirs_all = dir_semistr.split(";" , QString::SplitBehavior::SkipEmptyParts ) ; //qt5
        for ( QString qs_dir : qsl_dirs_all )
        {
            qs_dir = qs_dir.trimmed() ;
            if ( qs_dir.size() == 0 )
                continue ;
            if ( qs_dir[0] == '!' )
                qsl_dirs_exclude.push_back( QDir::toNativeSeparators ( qs_dir.mid( 1 ).trimmed() ) ) ;
            else
                qsl_dirs_root.push_back( QDir::toNativeSeparators( qs_dir ) ) ;
        }

        //Преобразуем вектора dirs_path в QStringList
        QStringList qsl_in_dirs_root;
        for (const QString & str : roots) {
            qsl_in_dirs_root.append( str );
        }
        QStringList qsl_in_dirs_exclude;
        for (const QString & str : exclude) {
            qsl_in_dirs_exclude.append( str );
        }
        //
        if ( qsl_dirs_root ==  qsl_in_dirs_root && qsl_dirs_exclude == qsl_in_dirs_exclude )
            return false ;

        for ( QString & qsdir1 : qsl_dirs_root )
        {
            // i.e. if root_dir is "c:" or "c:\" make it "c:\"
            if ( ! qsdir1.endsWith( QDir::separator() ) )
                qsdir1 = qsdir1 + QDir::separator() ;

            if ( ! QDir( qsdir1 ).exists() )
                throw ( qsdir1 ) ;
        }

        for ( QString & qsdir1 : qsl_dirs_exclude )
        {
            // i.e. if root_dir is "c:" or "c:\" make it "c:\"
            if ( ! qsdir1.endsWith( QDir::separator() ) )
                qsdir1 = qsdir1 + QDir::separator() ;

            if ( ! QDir( qsdir1 ).exists() )
                throw ( qsdir1 ) ;
        }

        // преобразуем в std::vector , засовываем обратно в вектора
        roots.clear() ;
        for (const QString& qstr : qsl_dirs_root ) {
            roots.push_back(qstr);
        }
        exclude.clear() ;
        for (const QString& qstr : qsl_dirs_exclude ) {
            exclude.push_back(qstr);
        }

        return true ;

    }

    // Вспомогательные методы
    bool isEmpty() const { return roots.empty() && exclude.empty(); }
    void clear() { roots.clear(); exclude.clear(); }

    // Методы для работы с roots
    void addRoot(const QString & root) {
        if (!contains(roots, root)) roots.push_back(root);
    }
    void removeRoot(const QString & root) {
        roots.erase(std::remove(roots.begin(), roots.end(), root), roots.end());
    }

    // Методы для работы с exclude
    void addExclude(const QString & dir) {
        if (!contains(exclude, dir)) exclude.push_back(dir);
    }
    void removeExclude(const QString & dir) {
        exclude.erase(std::remove(exclude.begin(), exclude.end(), dir), exclude.end());
    }

    QString toString() {
        QString res = "Корневые папки:";
        for ( QString const & dir : roots )
            res+= " " + dir + " ;";
        bool first = true ;
        for ( QString const & dir : exclude ) {
            if ( first ) {
                first = false;
                res += "  Искл.:";
            }
            res+= " " + dir + " ;";
        }
        return res ;
    }

    struct ValidationResult {
        bool isValid = false;
        std::vector<QString> errors;
        std::vector<QString> warnings;
    };

    ValidationResult validate() const {
        ValidationResult result;

        // Проверка обязательных полей
        if (roots.empty()) {
            result.warnings.push_back( "Не введены корневые директории" ) ;
        }

        // Проверка уникальности roots
        std::set<QString> uniqueRoots(roots.begin(), roots.end());
        if (uniqueRoots.size() != roots.size()) {
            result.warnings.push_back( "Корневые директории должно быть уникальны" );
        }

        // Проверка что root не содержат subroot,
        // напр. если ищем в /home , то в /home/dave искать не надо
        for ( int i = 0 ; i < static_cast<int>( roots.size() ) - 1 ; ++i )
            for ( int j = i+1 ; j < roots.size() ; ++j ) {
                if ( roots.at(i).contains( QString(roots.at(j)).replace('\\','/') ) )
                    result.warnings.push_back( "Поиск в поддиректории " + roots.at(i) + " уже задан в " + roots.at(j) ) ;
                if ( roots.at(j).contains( QString(roots.at(i)).replace('\\','/') ) )
                    result.warnings.push_back( "Поиск в поддиректории " + roots.at(j) + " уже задан в " + roots.at(i) ) ;
            }


        // Проверка что exclude не содержат roots
        // for (const auto& root : roots) {
        //     for (const auto& excludeDir : exclude) {
        //         if (excludeDir.find(root) == 0) {
        //             result.warnings.push_back(
        //                 "Excluded directory '" + excludeDir +
        //                 "' is inside root directory '" + root + "'"
        //                 );
        //         }
        //     }
        // }

        result.isValid = result.errors.empty() & result.warnings.empty() ;
        return result;
    }

private:
    bool contains(const std::vector<QString>& vec, const QString & value) const {
        return std::find(vec.begin(), vec.end(), value) != vec.end() ;
    }
};

} // namespace application::dto


#endif // DIRSCONF_H
