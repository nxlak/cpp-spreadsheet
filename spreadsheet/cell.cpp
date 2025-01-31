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


// Реализуйте следующие методы
Cell::Cell(Sheet& sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> temp_impl;

    if (text.empty()) {
        temp_impl = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        temp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        temp_impl = std::make_unique<TextImpl>(std::move(text));
    }

    HasCircularDependency(*temp_impl);

    impl_ = std::move(temp_impl);

    FillDependets();

    ClearCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
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

void Cell::HasCircularDependency(Impl& impl) {
    std::unordered_set<const Cell*> references;
    
    for (const Position& pos : impl.GetReferencedCells()) {
        references.insert(sheet_.GetConcreteCell(pos));
    }

    std::stack<const Cell*> visit;
    std::unordered_set<const Cell*> visited;

    visit.push(this);

    while(!visit.empty()) {
        const Cell* ongoing = visit.top();
        visit.pop();
        visited.insert(ongoing);

        if (references.count(ongoing)) {
            throw CircularDependencyException("last add cell make circular dependency");
        }

        for (const Cell* dependet : ongoing->dependets_) {
            visit.push(dependet);
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
    impl_->ClearCache();
    for (Cell* cell : dependets_) {
        cell->ClearCache();
    }
}

bool Cell::Impl::HasCache() {
    return true;
}

void Cell::Impl::ClearCache() {
    return;
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

Cell::TextImpl::TextImpl(std::string value) : value_(std::move(value)) {}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (value_[0] == ESCAPE_SIGN) {
        return value_.substr(1);
    }
    return value_;
}

Cell::FormulaImpl::FormulaImpl(std::string value, const SheetInterface& sheet) : formula_(ParseFormula(value.substr(1))), sheet_(sheet) {}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet_);
    }

    if (std::holds_alternative<double>(cache_.value())) {
        return std::get<double>(cache_.value());
    }
    return std::get<FormulaError>(cache_.value());
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