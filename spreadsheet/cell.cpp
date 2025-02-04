#include "cell.h"
#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <optional>
#include <unordered_set>
#include <variant>

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    // 1) Сохраняем предыдущие ссылки
    std::vector<Position> old_refs = GetReferencedCells();

    // 2) Создаём новую реализацию Impl в зависимости от входного текста
    std::unique_ptr<Impl> temp_impl;
    if (text.empty()) {
        temp_impl = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        temp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        temp_impl = std::make_unique<TextImpl>(std::move(text));
    }

    // 3) Проверяем на появление циклической зависимости
    ThrowIfCircularDependency(*temp_impl);

    // 4) Удаляем текущую ячейку из множеств dependets_ у тех ячеек,
    //    на которые мы ссылались раньше
    for (const Position& pos : old_refs) {
        if (Cell* old_ref_cell = sheet_.GetConcreteCell(pos)) {
            old_ref_cell->dependets_.erase(this);
        }
    }

    // 5) Присваиваем новую реализацию
    impl_ = std::move(temp_impl);

    // 6) Регистрируемся в зависимостях новых ячеек
    FillDependets();

    // 7) Сбрасываем кэш (у текущей и всех зависящих от неё)
    ClearCache();
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependets_.empty();
}

// Вспомогательный метод
bool Cell::CheckCircularDependency(const Cell* current, const Cell* target,
                                   std::unordered_set<const Cell*>& visited) const {
    if (!current) {
        return false;
    }
    if (current == target) {
        return true;
    }
    if (!visited.insert(current).second) {
        // Уже посещали
        return false;
    }

    for (const Position& pos : current->GetReferencedCells()) {
        const Cell* next = sheet_.GetConcreteCell(pos);
        if (CheckCircularDependency(next, target, visited)) {
            return true;
        }
    }
    return false;
}

void Cell::ThrowIfCircularDependency(Impl& impl) {
    std::vector<Position> new_refs = impl.GetReferencedCells();
    std::unordered_set<const Cell*> visited;

    for (const Position& pos : new_refs) {
        const Cell* ref_cell = sheet_.GetConcreteCell(pos);
        if (CheckCircularDependency(ref_cell, this, visited)) {
            throw CircularDependencyException("Circular dependency detected");
        }
    }
}

void Cell::FillDependets() {
    for (const Position& pos : impl_->GetReferencedCells()) {
        Cell* cell = sheet_.GetConcreteCell(pos);
        if (!cell) {
            sheet_.SetCell(pos, "");
            cell = sheet_.GetConcreteCell(pos);
        }
        cell->dependets_.insert(this);
    }
}

void Cell::ClearCache() {
    std::unordered_set<Cell*> visited;
    std::stack<Cell*> stack;
    stack.push(this);

    while (!stack.empty()) {
        Cell* c = stack.top();
        stack.pop();

        if (!visited.insert(c).second) {
            // Уже сбрасывали
            continue;
        }
        c->impl_->ClearCache();

        for (Cell* dep : c->dependets_) {
            stack.push(dep);
        }
    }
}

bool Cell::Impl::HasCache() {
    return true;
}

void Cell::Impl::ClearCache() {
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}

Cell::TextImpl::TextImpl(std::string value)
    : value_(std::move(value)) {
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (!value_.empty() && value_.front() == ESCAPE_SIGN) {
        return value_.substr(1);
    }
    return value_;
}

Cell::FormulaImpl::FormulaImpl(std::string value, const SheetInterface& sheet)
    : formula_(ParseFormula(value.substr(1)))
    , sheet_(sheet) {
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet_);
    }
    if (std::holds_alternative<double>(*cache_)) {
        return std::get<double>(*cache_);
    }
    return std::get<FormulaError>(*cache_);
}

bool Cell::FormulaImpl::HasCache() {
    return cache_.has_value();
}

void Cell::FormulaImpl::ClearCache() {
    cache_.reset();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}
