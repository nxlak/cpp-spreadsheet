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

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        if (int(table_.size()) <= pos.row) {
            table_.resize(pos.row + 1);
        }
        if (int(table_[pos.row].size()) <= pos.col) {
            table_[pos.row].resize(pos.col + 1);
        }
        if (!table_[pos.row][pos.col]) {
            table_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        }
        table_[pos.row][pos.col]->Set(std::move(text));
    } else {
        throw InvalidPositionException("SetCell error pos");
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (pos.row < int(table_.size()) && pos.col < int(table_[pos.row].size())) {
            if (!table_[pos.row][pos.col]->GetText().empty()) {
                return table_[pos.row][pos.col].get();
            }
        }
        return nullptr;
    } else {
        throw InvalidPositionException("GetCell error pos");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (pos.IsValid()) {
        if (pos.row < int(table_.size()) && pos.col < int(table_[pos.row].size())) {
            return table_[pos.row][pos.col].get();
        }
        return nullptr;
    } else {
        throw InvalidPositionException("GetConcreteCell error pos");
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        if (pos.row < int(table_.size()) && pos.col < int(table_[pos.row].size())) {
            if (table_[pos.row][pos.col]) {
                table_[pos.row][pos.col]->Clear();
            }
        }
    } else {
        throw InvalidPositionException("ClearCell error pos");
    }
}

Size Sheet::GetPrintableSize() const {
    Size size_;
    
    for (int i = 0; i < int(table_.size()); ++i) {
        for (int j = table_[i].size() - 1; j >= 0; --j) {
            if (table_[i][j]) {
                if (!table_[i][j]->GetText().empty()) {
                    size_.rows = std::max(i + 1, size_.rows);
                    size_.cols = std::max(j + 1, size_.cols);
                    break;
                }
            }
        }
    }

    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; ++i) {
        for (int j = 0; j < GetPrintableSize().cols; ++j) {
            if (j) {
                output << '\t';
            }
            if (j < int(table_[i].size())) {
                if (table_[i][j]) {
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
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; ++i) {
        for (int j = 0; j < GetPrintableSize().cols; ++j) {
            if (j) {
                output << '\t';
            }
            if (j < int(table_[i].size())) {
                if (table_[i][j]) {
                    output << table_[i][j]->GetText();
                }
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}