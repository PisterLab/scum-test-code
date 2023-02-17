#include "matrix.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Validate that the given row and column are in bounds.
static inline bool matrix_validate(const matrix_t* matrix, const size_t row,
                                   const size_t col) {
    return row < matrix->rows && col < matrix->cols;
}

// Return the 1D array index corresponding to the given row and column.
static inline size_t matrix_index(const matrix_t* matrix, const size_t row,
                                  const size_t col) {
    return row * matrix->cols + col;
}

bool matrix_init(matrix_t* matrix, const size_t rows, const size_t cols) {
    if (rows * cols > MATRIX_MAX_SIZE) {
        return false;
    }

    matrix->rows = rows;
    matrix->cols = cols;

    // Zero-initialize the matrix.
    memset(matrix->buffer, 0, MATRIX_MAX_SIZE * sizeof(matrix_type_t));
    return true;
}

size_t matrix_num_rows(const matrix_t* matrix) { return matrix->rows; }

size_t matrix_num_columns(const matrix_t* matrix) { return matrix->cols; }

bool matrix_get(const matrix_t* matrix, const size_t row, const size_t col,
                matrix_type_t* element) {
    if (!matrix_validate(matrix, row, col)) {
        return false;
    }

    *element = matrix->buffer[matrix_index(matrix, row, col)];
    return true;
}

bool matrix_set(matrix_t* matrix, const size_t row, const size_t col,
                const matrix_type_t value) {
    if (!matrix_validate(matrix, row, col)) {
        return false;
    }

    matrix->buffer[matrix_index(matrix, row, col)] = value;
    return true;
}

bool matrix_add(const matrix_t* matrix1, const matrix_t* matrix2,
                matrix_t* result) {
    const size_t num_rows = matrix1->rows;
    const size_t num_cols = matrix1->cols;

    // Validate that the input matrices have the same size.
    if (num_rows != matrix2->rows || num_cols != matrix2->cols) {
        return false;
    }

    // Initialize the result matrix.
    if (!matrix_init(result, num_rows, num_cols)) {
        return false;
    }

    // Add the two input matrices element-wise.
    for (size_t i = 0; i < num_rows * num_cols; ++i) {
        result->buffer[i] = matrix1->buffer[i] + matrix2->buffer[i];
    }
    return true;
}

bool matrix_multiply(const matrix_t* matrix1, const matrix_t* matrix2,
                     matrix_t* result) {
    const size_t num_rows = matrix1->rows;
    const size_t num_cols = matrix2->cols;
    const size_t inner_dimension = matrix1->cols;

    // Validate that the input matrices have compatible dimensions.
    if (inner_dimension != matrix2->rows) {
        return false;
    }

    // Initialize the result matrix.
    if (!matrix_init(result, num_rows, num_cols)) {
        return false;
    }

    // Multiple the two input matrices.
    for (size_t i = 0; i < num_rows; ++i) {
        for (size_t j = 0; j < num_cols; ++j) {
            for (size_t k = 0; k < inner_dimension; ++k) {
                result->buffer[matrix_index(result, i, j)] +=
                    matrix1->buffer[matrix_index(matrix1, i, k)] *
                    matrix2->buffer[matrix_index(matrix2, k, j)];
            }
        }
    }
    return true;
}
