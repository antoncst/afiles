#ifndef HISTORYNAVIGATOR_HPP
#define HISTORYNAVIGATOR_HPP

#include <deque>
#include <vector>
#include <cassert>
#include <cstddef> // для size_t

template<class T>
class historyNavigator
{
    std::deque<T> content_;
    mutable size_t cur_index_ = 0;  // индекс текущего элемента (0 - нет элементов)
    size_t max_size_ = 0;   // 0 - unlimited size

public:
    historyNavigator(size_t max_size = 0) : max_size_(max_size) {}

    // Добавить новый элемент
    void add(const T& item) {
        // Проверяем, не является ли новый элемент таким же как текущий
        if (cur_index_ > 0) {
            if (content_[cur_index_ - 1] == item) {
                return; // не добавляем одинаковые элементы
            }
        }

        // Удаляем "будущее", если двигались назад
        if (cur_index_ < content_.size()) {
            content_.resize(cur_index_);
        }

        assert(cur_index_ == content_.size());
        content_.push_back(item);
        cur_index_++;

        // Ограничиваем размер истории
        if (max_size_ > 0) {
            while (content_.size() > max_size_) {
                content_.pop_front();
                cur_index_--;
            }
        }
    }

    // Назад в истории
    T get_back() const {
        if (cur_index_ > 1) {
            --cur_index_;
        }

        if (cur_index_ > 0) {
            return content_[cur_index_ - 1];
        }
        return T(); // возвращаем значение по умолчанию
    }

    // Вперед в истории
    T get_forward() const {
        if (cur_index_ < content_.size()) {
            ++cur_index_;
        }

        if (cur_index_ > 0) {
            return content_[cur_index_ - 1];
        }
        return T();
    }

    // Получить текущий элемент
    T get_current() const {
        if (cur_index_ > 0) {
            return content_[cur_index_ - 1];
        }
        return T(); // или бросить исключение
    }

    // Проверить, можно ли идти назад
    bool has_back() const {
        return cur_index_ > 1;
    }

    // Проверить, можно ли идти вперед
    bool has_forward() const {
        return cur_index_ < content_.size();
    }

    // Проверить, есть ли текущий элемент
    bool has_current() const {
        return cur_index_ > 0;
    }

    // Получить копию истории как вектор
    std::vector<T> get_content() const {
        return std::vector<T>(content_.begin(), content_.end());
    }

    // Установить историю из вектора
    void set_content(const std::vector<T>& vcontent) {
        content_ = std::deque<T>(vcontent.begin(), vcontent.end());
        // Корректируем текущий индекс
        if (cur_index_ > content_.size()) {
            cur_index_ = content_.size();
        }
    }

    // Получить текущий индекс (1-based)
    size_t get_current_index() const {
        return cur_index_; // возвращает 0 если нет элементов
    }

    // Установить текущий элемент по индексу (1-based)
    bool set_current_index(size_t index) {
        if (index == 0 || index > content_.size()) {
            return false; // недопустимый индекс
        }
        cur_index_ = index;
        return true;
    }

    // Установить максимальный размер
    void set_max_size(size_t max_size) {
        max_size_ = max_size;
        // При необходимости обрезаем существующую историю
        if (max_size_ > 0 && content_.size() > max_size_) {
            // Определяем сколько элементов нужно удалить с начала
            size_t to_remove = content_.size() - max_size_;

            // Удаляем элементы
            for (size_t i = 0; i < to_remove; ++i) {
                content_.pop_front();
            }

            // Корректируем текущий индекс
            if (cur_index_ > to_remove) {
                cur_index_ -= to_remove;
            } else {
                cur_index_ = 0;
            }
        }
    }

    // Очистить историю
    void clear() {
        content_.clear();
        cur_index_ = 0;
    }

    // Получить размер истории
    size_t size() const {
        return content_.size();
    }

    // Проверить, пуста ли история
    bool empty() const {
        return content_.empty();
    }

    // Получить максимальный размер
    size_t get_max_size() const {
        return max_size_;
    }
};

#endif // HISTORYNAVIGATOR_HPP
