#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()), 
      sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> temp_impl;

    if (text[0] == '=' && text.size() > 1) {
        temp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }else if (text.size() == 0) {
        temp_impl = std::make_unique<EmptyImpl>();
    }else {
        temp_impl = std::make_unique<TextImpl>(std::move(text));
    }

    if (HasCircularDependence(temp_impl->GetReferencedCells())) {
        throw CircularDependencyException("Circular dependency exception");
    }
    impl_ = std::move(temp_impl);

    UpdateCellsDependence(impl_->GetReferencedCells());

    InvalidateCellCache(true);
}

void Cell::Clear() {
    Set(std::string());
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
    return !dependent_cells_.empty();
}

void Cell::InvalidateCellCache(bool flag) {
    if (impl_->HasCache() || flag) {
        impl_->InvalidateCache();

        for (Cell* dep_cell : dependent_cells_) {
            dep_cell->InvalidateCellCache();
        }
    }
}

bool Cell::HasCircularDependence(const std::vector<Position>& ref_cells_pos) {
    if (ref_cells_pos.empty()) {
        return false;
    }

    std::unordered_set<const Cell*> ref_cells;
    std::unordered_set<const Cell*> visited_cells;
    std::vector<const Cell*> cells_to_visit;

    for (const auto& pos : ref_cells_pos) {
        ref_cells.insert(sheet_.GetCellPtr(pos));
    }

    cells_to_visit.push_back(this);

    while (!cells_to_visit.empty()) {
        const Cell* curr_cell = cells_to_visit.back();
        cells_to_visit.pop_back();
        visited_cells.insert(curr_cell);

        if (ref_cells.find(curr_cell) != ref_cells.end()) {
            return true;
        }
        for (const Cell* dep_cell : curr_cell->dependent_cells_) {
            if (visited_cells.find(dep_cell) == visited_cells.end()) {
                cells_to_visit.push_back(dep_cell);
            }
        }
    }
    return false;
}

void Cell::UpdateCellsDependence(const std::vector<Position>& ref_cell_pos) {
    for (Cell* cell : referenced_cells_) {
        cell->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();

    for (const auto& pos : ref_cell_pos) {
        Cell* ref_cell = sheet_.GetCellPtr(pos);

        if (!ref_cell) {
            sheet_.SetCell(pos, std::string());
            ref_cell = sheet_.GetCellPtr(pos);
        }
        referenced_cells_.insert(ref_cell);
        ref_cell->dependent_cells_.insert(this);
    }
}