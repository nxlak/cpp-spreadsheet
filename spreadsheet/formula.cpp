#include "formula.h"

#include "FormulaAST.h"
#include "common.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <forward_list>
#include <functional>
#include <iterator>
#include <sstream>
#include <variant>
#include <vector>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {}
    catch (...) {
        throw FormulaException("syntactically incorrect formula");
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> args = [&sheet](const Position& pos) -> double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            const CellInterface* cell = sheet.GetCell(pos);

            if (!cell) {
                return 0.0;
            }

            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }

            if (std::holds_alternative<std::string>(cell->GetValue())) {
                std::string cell_str = std::get<std::string>(cell->GetValue());

                double num = 0.0;

                if (cell_str == "") {
                    return num;
                }

                std::istringstream str(cell_str);
                if (str >> num && str.eof()) {
                    return num;
                }

                throw FormulaError(FormulaError::Category::Value);
            }

            throw std::get<FormulaError>(cell->GetValue());
        };

        try {
            return ast_.Execute(args);
        } catch (const FormulaError& error) {
            return error;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        
        const std::forward_list<Position>& list = ast_.GetCells();
        result.reserve(std::distance(list.begin(), list.end()));

        for (auto& cell : list) {
            if (cell.IsValid()) {
                result.push_back(cell);
            }
        }

        auto it_end = std::unique(result.begin(), result.end());
        result.resize(it_end - result.begin());

        return result;
    }
private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}