#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <windows.h>
#include <errno.h>
#include <iomanip>
#include <string>

using namespace std;

union ErrData {
    struct Elem {
        int RowIdx;
        int ColIdx;
        string* Token;
    } element;
    int rowIdx;
};

enum error {
    no_error,
    incorrect_double,
    non_rectangular,
    no_filename,
    empty_matrix,
    mem_allocate,
};

error check_file(ifstream &f, int &rowsCount, int &colsCount, ErrData *err_data = NULL) {
    string line;
    char *p;

    int matrixColNumber = 0;
    int matrixRowNumber = 0;

    while (getline(f, line)) {
        int colNumber = 0;
        stringstream s(line);

        do {
            string token;
            s >> token;

            if (s.fail() && s.eof()) {
                break;
            }

            double d;
            stringstream token_s{token};
            token_s >> d;

            if (token_s.fail() || !token_s.eof()) {
                if (err_data != NULL) {
                    err_data->element.RowIdx = matrixRowNumber + 1;
                    err_data->element.ColIdx = colNumber + 1;
                    *err_data->element.Token = token;
                }
                return incorrect_double;
            }

            colNumber++;
        } while (true);

        if (colNumber == 0) {
            // skip empty line
            continue;
        }

        matrixRowNumber++;

        if (matrixColNumber == 0) {
            matrixColNumber = colNumber;
        }

        if (matrixColNumber != colNumber) {
            if (err_data != NULL) {
                err_data->rowIdx = matrixRowNumber;
            }
            return non_rectangular;
        }
    }

    rowsCount = matrixRowNumber;
    colsCount = matrixColNumber;

    return no_error;
}

error allocate_matrix(double** &matrix, int rowsCount, int colsCount) {
    int i = 0;
    try {
        matrix = new double *[rowsCount];
    } catch (bad_alloc&) {
        return mem_allocate;
    }
    try {
        for (; i < rowsCount; ++i) {
            matrix[i] = new double[colsCount];
        }
    } catch (bad_alloc&) {
        for (int j; j < i; ++j) {
            delete[] matrix[j];
        }
        return mem_allocate;
    }
    return no_error;
}

void free_matrix(double** &matrix, int rowsCount) {
    for (int i = 0; i < rowsCount; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
    matrix = NULL;
}

void read_matrix(ifstream &f, double** matrix, int rowsCount, int colsCount) {
    for (int i = 0; i < rowsCount; i++) {
        for (int j = 0; j < colsCount; j++) {
            f >> *(*(matrix + i) + j);
        }
    }
}

string format_matrix(double** matrix, int RowCount, int ColumnCount, int precision, int colwidth = 10) {
    stringstream ss;
    for (int i = 0; i < RowCount; i++) {
        for (int j = 0; j < ColumnCount; j++) {
            ss << fixed << setw(colwidth) << setprecision(precision) << matrix[i][j];
        }
        ss << endl;
    }
    return ss.str();
}

error process_matrix(ErrData *err_data = NULL) {

    ifstream file;

    do {
        string filename;
        cout << "Введите название файла или * для выхода из программы" << endl;
        getline(cin, filename);
        if (filename == "*") {
            return no_filename;
        }

        file.open(filename, ios::in);
        if (file.fail()) {
            cout << "Не удалось открыть файл: " << strerror(errno) << endl;
            continue;
        }

        break;
    } while (true);

    int rowsCount;
    int colsCount = 0;
    error check_result = check_file(file, rowsCount, colsCount, err_data);
    if (check_result != no_error) {
        return check_result;
    }

    if (rowsCount < 1 || colsCount < 1) {
        return empty_matrix;
    }

    double** matrix;
    error allocate_error = allocate_matrix(matrix, rowsCount, colsCount);
    if (allocate_error != no_error) {
        return allocate_error;
    }


    file.clear();
    file.seekg(0);
    read_matrix(file, matrix, rowsCount, colsCount);

    cout << "Оригинальная матрица:" << endl;
    cout << format_matrix(matrix, rowsCount, colsCount, 2);

    int minDimension = rowsCount;
    if (colsCount < rowsCount) {
        minDimension = colsCount;
    }

    double maxDiagElement;
    int maxDiagElementColNumber;
    for (int i = 0; i < minDimension; i++) {
        if (i == 0) {
            maxDiagElement = matrix[i][i];
            maxDiagElementColNumber = 0;
            continue;
        }

        if (matrix[i][i] > maxDiagElement) {
            maxDiagElement = matrix[i][i];
            maxDiagElementColNumber = i;
        }
    }

    if (abs(maxDiagElement) < 5) {
        double lastElement = matrix[rowsCount - 1][colsCount - 1];
        double mult = pow(lastElement, 2);

        for (int i = 0; i < rowsCount; ++i) {
            for (int j = 0; j < colsCount; ++j) {
                if (j == maxDiagElementColNumber) {
                    continue;
                }

                matrix[i][j] *= mult;
            }
        }
    }

    cout << "Результат: " << endl;
    cout << format_matrix(matrix, rowsCount, colsCount, 2);

    free_matrix(matrix, rowsCount);
    return no_error;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    ErrData err_data;
    string err_data_err_token;
    err_data.element.Token = &err_data_err_token;
    error err = process_matrix(&err_data);
    if (err != no_error) {
        switch (err) {
            case no_filename: {
                cout << "Не задано имя файла" << endl;
                return 1;
            }
            case incorrect_double: {
                cout << "Некорректный элемент матрицы, ряд " << err_data.element.RowIdx << ", колонка "
                     << err_data.element.ColIdx << ": \"" << *err_data.element.Token << "\"" << endl;
                return 1;
            }
            case non_rectangular: {
                cout << "Матрица не является прямоугольной, некорректный ряд " << err_data.rowIdx << endl;
                return 1;
            }
            case empty_matrix: {
                cout << "Файл содержит пустую матрицы" << endl;
                return 1;
            }
            case mem_allocate: {
                cout << "Неуспешное выделение памяти" << endl;
                return 1;
            }
            case no_error: {
                break;
            }
        }
    }
    return 0;
}
