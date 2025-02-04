#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>

using namespace std::literals;

Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {
    // Проверяем корректность позиции
    if (!pos.IsValid()) {
        throw InvalidPositionException("SetCell error pos");
    }
    // Расширяем вектор строк при необходимости
    if (static_cast<size_t>(pos.row) >= table_.size()) {
        table_.resize(pos.row + 1);
    }
    // Расширяем строку-список ячеек при необходимости
    if (static_cast<size_t>(pos.col) >= table_[pos.row].size()) {
        table_[pos.row].resize(pos.col + 1);
    }

    // Создаём ячейку, если её до сих пор не было
    if (!table_[pos.row][pos.col]) {
        table_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }

    // Устанавливаем новое содержимое
    table_[pos.row][pos.col]->Set(std::move(text));
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("GetCell error pos");
    }
    if (static_cast<size_t>(pos.row) < table_.size() &&
        static_cast<size_t>(pos.col) < table_[pos.row].size() &&
        table_[pos.row][pos.col]) {
        // Если ячейка существует, но её текст пуст — возвращаем nullptr
        if (!table_[pos.row][pos.col]->GetText().empty()) {
            return table_[pos.row][pos.col].get();
        }
    }
    return nullptr;
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("GetConcreteCell error pos");
    }
    if (static_cast<size_t>(pos.row) < table_.size() &&
        static_cast<size_t>(pos.col) < table_[pos.row].size()) {
        return table_[pos.row][pos.col].get();
    }
    return nullptr;
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("ClearCell error pos");
    }
    if (static_cast<size_t>(pos.row) < table_.size() &&
        static_cast<size_t>(pos.col) < table_[pos.row].size() &&
        table_[pos.row][pos.col]) {
        table_[pos.row][pos.col]->Clear();

        // Если после очистки ячейка не имеет внешних ссылок и её текст пуст —
        // можно удалить объект, чтобы она не висела пустой
        if (!table_[pos.row][pos.col]->IsReferenced() &&
            table_[pos.row][pos.col]->GetText().empty()) {
            table_[pos.row][pos.col].reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;
    for (int i = 0; i < static_cast<int>(table_.size()); ++i) {
        for (int j = static_cast<int>(table_[i].size()) - 1; j >= 0; --j) {
            if (table_[i][j]) {
                if (!table_[i][j]->GetText().empty()) {
                    size.rows = std::max(size.rows, i + 1);
                    size.cols = std::max(size.cols, j + 1);
                    break;
                }
            }
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << '\t';
            }
            if (static_cast<size_t>(i) < table_.size() &&
                static_cast<size_t>(j) < table_[i].size() &&
                table_[i][j]) {
                auto value = table_[i][j]->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                } else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                } else if (std::holds_alternative<FormulaError>(value)) {
                    output << std::get<FormulaError>(value);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << '\t';
            }
            if (static_cast<size_t>(i) < table_.size() &&
                static_cast<size_t>(j) < table_[i].size() &&
                table_[i][j]) {
                output << table_[i][j]->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
