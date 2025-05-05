# Spreadsheet

**Spreadsheet** is a simplified analogue of existing spreadsheet solutions such as *Microsoft Excel* or *Google Sheets*. Table cells can contain either plain text or formulas. Formulas can include cell indexes, similar to the aforementioned products. This spreadsheet project is memory-efficient (especially for sparse tables where the printable area is large relative to the number of non-empty cells), avoids memory leaks when cells or the entire sheet are deleted, and provides `O(1)` access to cells by index.

## Overview

### Cells and Indexes

The table stores `Cell` objects. For users, a cell is accessed by its index in the format `A1`, `C14`, or `RD2`, where `A1` represents the top-left corner of the sheet.

The number of rows and columns in the spreadsheet does not exceed **16384**, meaning the maximum valid cell index is `(16383, 16383)` corresponding to `XFD16384`. If a cell position exceeds these boundaries, it is considered invalid. The ***position*** structure is defined in the `common.h` file and contains `col` and `row` fields — the column and row indices used for cell access.

### Minimal Printable Area

To print the spreadsheet, the size of the minimal printable area must be known. This is the smallest rectangular area starting from `A1` that includes all non-empty cells.

The `Size` structure is defined in `common.h` and contains the number of rows and columns in the minimal printable area.

### Methods Accessing Cells by Index

* `SetCell(Position, std::string)` – sets the content of a cell at the given `Position`. If the cell is empty, it is created. The text is assigned via `Cell::Set(std::string)`;
* `Cell* GetCell(Position pos)` – const and non-const getters that return a pointer to the cell at `pos`. If the cell is empty, returns `nullptr`;
* `void ClearCell(Position pos)` – clears the content of the cell at the given index. Subsequent `GetCell()` calls will return `nullptr`. This may also change the size of the printable area.

### Methods for the Whole Sheet

* `Size GetPrintableSize()` – determines the size of the minimal printable area. The `Size` structure in `common.h` defines this;
* `void PrintText(std::ostream&)` – prints the textual representation of cells:
  * For text cells – prints the raw text set by `Set()`, including leading apostrophes `'`;
  * For formula cells – prints the formula with simplified brackets using `Formula::GetExpression()` and a leading `=`.
* `void PrintValues(std::ostream&)` – prints the evaluated cell values — strings, numbers, or `FormulaError` — as defined in `Cells::GetValue()`.

### Evaluating Cell Values

For example, cell `A3` contains a formula `=1+2*7`, which evaluates to `15`. Cell `A2` contains the plain text `"3"`, which is not a formula but can be interpreted as a number, so its value is `3`. Cell `C2` contains the formula `=A3/A2`, which divides the value of `A3` by `A2`: 15 / 3 = **5**.

If a formula references an empty cell, the empty cell’s value is considered to be `0`.

## Exception Handling

### Evaluation Errors

Evaluation may result in errors, e.g., division by zero. If a divisor is zero, the result is a `FormulaError` of type **#DIV/0!**.

If a referenced cell cannot be interpreted as a number, a `FormulaError` **#VALUE!** is returned.

If a formula references a cell that is out of bounds, like `=A1234567+ZZZZ1`, it can be created but not evaluated, resulting in an error **#REF!**.

Errors propagate through dependencies. If a formula depends on multiple cells, each with errors, the result may be any of those errors.

### Invalid Formula

If a syntactically invalid formula (e.g. `=A1+*`) is passed to `Sheet::SetCell()`, a `FormulaException` is thrown, and the cell value remains unchanged. A formula is invalid if it doesn't follow standard grammar rules.

### Invalid Position

A `Position` instance with invalid coordinates (e.g. `(-1, -1)`) can be created programmatically. If such a position is used in a method, it throws an `InvalidPositionException`.

### Circular Dependencies

Circular dependencies are not allowed. If cells form a cycle, their values cannot be computed. Attempting to set a formula that introduces a cycle throws a `CircularDependencyException`, and the cell value is not changed.

## System Requirements

1. C++ 17
2. GCC (MinGW-w64) 11+
3. JDK – Java Development Kit https://www.oracle.com/java/technologies/downloads/
4. ANTLR4 library https://www.antlr.org/
5. CMake version 3.8+ https://cmake.org/

## Installation Steps

1. Install CMake and JDK.
2. Install ANTLR. Installation instructions can be found at https://www.antlr.org/.
3. Create `Debug` and `Release` directories if they do not exist:

```
mkdir ../Debug
mkdir ../Release
```

4. Navigate to the `spreadsheet` directory and run the following commands for `Debug` and/or `Release` builds:

```
cmake -E chdir ../Debug/ cmake -G "Unix Makefiles" ../spreadsheet/ -DCMAKE_BUILD_TYPE:STRING=Debug
cmake -E chdir ../Release/ cmake -G "Unix Makefiles" ../spreadsheet/ -DCMAKE_BUILD_TYPE:STRING=Release 
```

5. Navigate to the `Debug` or `Release` directory and build the project:

```
cmake --build .
```

6. Run the application:

```
./spreadsheet
```
