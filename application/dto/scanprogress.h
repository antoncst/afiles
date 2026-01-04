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
        return QString("Обработано: %1 | Текущий: %2").arg(current).arg(currentItem);
    }
};

} // namespace application::dto


#endif // SCANPROGRESS_H
