#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <set>

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
    void InvalidateCellCache(bool flag);

private:
    class Impl {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;

        virtual std::vector<Position> GetReferencedCells() const {
            return {};
        }
        virtual bool HasCache() {
            return true;
        }
        virtual void InvalidateCache() {}
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() {
            value_ = text_ = "";
        }

        Value GetValue() const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }

    private: 
        Value value_;
        std::string text_;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text) {
            if (text[0] == '\'') {
                value_ = text.substr(1);
            }else {
                value_ = text;
            }
            text_ = std::move(text);
        }

        Value GetValue() const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }

    private:
        Value value_;
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public: 
        explicit FormulaImpl(std::string text, SheetInterface& sheet) 
            : formula_ptr_(ParseFormula(text.substr(1))), sheet_(sheet) {
        }

        Value GetValue() const override {
            if (!cache_) {
                cache_ = formula_ptr_->Evaluate(sheet_);
            }
            if (std::holds_alternative<double>(cache_.value())) {
                return std::get<double>(cache_.value());
            }
            return std::get<FormulaError>(cache_.value());
        }

        std::string GetText() const override {
            return '=' + formula_ptr_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const override {
            return formula_ptr_->GetReferencedCells();
        }

    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;  
        mutable std::optional<FormulaInterface::Value> cache_;
        SheetInterface& sheet_;
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::set<Cell*> dependent_cells_;
    std::set<Cell*> referenced_cells_;

    bool HasCircularDependence(const std::vector<Position>& ref_cells_pos);
    void UpdateCellsDependence(const std::vector<Position>& ref_cells_pos);
};    
