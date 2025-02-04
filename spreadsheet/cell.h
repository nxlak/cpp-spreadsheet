#pragma once

#include "common.h"
#include "formula.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    // Ячейки, которые ссылаются на данную ячейку
    std::unordered_set<Cell*> dependets_;

    void ThrowIfCircularDependency(Impl& impl);

    void FillDependets();
    void ClearCache();

    // Вспомогательный метод для проверки циклических зависимостей
    bool CheckCircularDependency(const Cell* current, const Cell* target,
                                 std::unordered_set<const Cell*>& visited) const;

    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;

        virtual bool HasCache();
        virtual void ClearCache();
        virtual std::vector<Position> GetReferencedCells() const;

        virtual ~Impl() = default;
    };

    class EmptyImpl : public Impl {
    public:
        std::string GetText() const override;
        Value GetValue() const override;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string value);
        std::string GetText() const override;
        Value GetValue() const override;
    private:
        std::string value_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string value, const SheetInterface& sheet);
        std::string GetText() const override;
        Value GetValue() const override;

        bool HasCache() override;
        void ClearCache() override;

        std::vector<Position> GetReferencedCells() const override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        const SheetInterface& sheet_;
        mutable std::optional<FormulaInterface::Value> cache_;
    };
};
