#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <sstream>

using namespace std::literals;

namespace {
    
class Formula : public FormulaInterface {
public:

    explicit Formula(std::string expression) 
       try : ast_(ParseFormulaAST(std::move(expression))) {
       } catch(...) {
            throw FormulaException("Incorrect formula syntax");
       }
        
    Value Evaluate(const SheetInterface& sheet) const override {

        std::function<double(Position)> args = [&sheet](const Position pos) {

            if (!pos.IsValid()) {
                throw FormulaError::Category::Ref;
            }
    
            const auto* cell = sheet.GetCell(pos);

            if (!cell) {
                return 0.;
            }

            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }

            if (std::holds_alternative<std::string>(cell->GetValue())) {
                std::string text = std::get<std::string>(cell->GetValue());

                if (text.empty()) {
                    return 0.;
                }
                std::istringstream input(text);
                double result = 0.;

                if (input >> result && input.eof()) {
                    return result;
                }else {
                    throw FormulaError::Category::Value;
                }
            }
            throw std::get<FormulaError>(cell->GetValue());
        };

        try {
            return ast_.Execute(args);
        }catch (const FormulaError& err) {
            return err;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid()) cells.push_back(cell);
        }
        return cells;
    }

    std::string GetExpression() const override {
        std::ostringstream output;
        ast_.PrintFormula(output);
        return output.str();
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("");
    }
}