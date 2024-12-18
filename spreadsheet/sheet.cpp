#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <variant>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    //позиция невалидна
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }
    //ячейка с такой позицией уже есть в таблице 
    if (cells_.find(pos) != cells_.end()) {
        //переназначаем ее
        cells_.at(pos)->Set(std::move(text));
    //ячейки с такой позицией нет в таблице
    }else {
        //создается ячейка
        cells_[pos] = std::make_unique<Cell>(*this);
        //добавление назначения ячейки
        cells_[pos]->Set(std::move(text));
        //добавление строки и колонки в счетчики
        max_cols_[pos.col] += 1;
        max_rows_[pos.row] += 1;
    }   
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    auto cell = cells_.find(pos);
    if (cell != cells_.end() && cell->second->GetText().size() != 0) {
        return cell->second.get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(static_cast<const Sheet&>(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    auto cell = cells_.find(pos);
    if (cell != cells_.end()) {
        cell->second->Clear();
        cells_.erase(pos);

        ChangePrintableSize(pos);
    }
}

void Sheet::ChangePrintableSize(Position pos) {
    if (max_cols_[pos.col] - 1 <= 0) {
        max_cols_.erase(pos.col);
    }else {
        max_cols_[pos.col] -= 1;
    }

    if (max_rows_[pos.row] - 1 <= 0) {
        max_rows_.erase(pos.row);
    }else {
        max_rows_[pos.row] -= 1;
    }
}

Size Sheet::GetPrintableSize() const {
    Size result {0, 0};
    if (max_cols_.size() != 0 && max_rows_.size() != 0) {
        result.cols = max_cols_.rbegin()->first + 1;
        result.rows = max_rows_.rbegin()->first + 1;
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        bool is_first = true;
        for (int col = 0; col < size.cols; ++col) {
            if (!is_first) {
                output << '\t';
            }

            is_first = false;
            Position pos {row, col};
            auto cell = GetCell(pos);

            if (cell) {
                std::visit([&output](const auto& result) {
                    output << result;}, cell->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        bool is_first = true;
        for (int col = 0; col < size.cols; ++col) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            Position pos {row, col};
            auto cell = GetCell(pos);

            if (cell) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    } 

    const auto cell = cells_.find(pos);
    if (cell == cells_.end()) {
        return nullptr;
    }
    return cells_.at(pos).get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}
