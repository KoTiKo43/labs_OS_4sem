#include <windows.h>
#include <iostream>
#include <vector>
#include <conio.h>
#include <limits>
#include <ios>
#include <string>

using namespace std;

struct Matrix {
    int rows;
    int cols;
    vector<vector<double>> data;
};

bool ReadMatrix(HANDLE hPipe, Matrix& matrix) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &matrix.rows, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int)) 
        return false;
    if (!ReadFile(hPipe, &matrix.cols, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int)) 
        return false;

    matrix.data.resize(matrix.rows, vector<double>(matrix.cols));
    for (auto& row : matrix.data) {
        for (auto& val : row) {
            if (!ReadFile(hPipe, &val, sizeof(double), &bytesRead, NULL) || bytesRead != sizeof(double))
                return false;
        }
    }
    return true;
}

bool WriteMatrix(HANDLE hPipe, const Matrix& matrix) {
    DWORD bytesWritten;
    if (!WriteFile(hPipe, &matrix.rows, sizeof(int), &bytesWritten, NULL) || bytesWritten != sizeof(int))
        return false;
    if (!WriteFile(hPipe, &matrix.cols, sizeof(int), &bytesWritten, NULL) || bytesWritten != sizeof(int))
        return false;

    for (const auto& row : matrix.data) {
        for (auto val : row) {
            if (!WriteFile(hPipe, &val, sizeof(double), &bytesWritten, NULL))
                return false;
        }
    }
    return true;
}

void SafeInput(int& value, const string& prompt) {
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail() || value <= 0) {
            cin.clear();
            cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            cerr << "Некорректный ввод! Повторите попытку" << endl;
            continue;
        }
        break;
    }
    cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);


    const DWORD TIMEOUT = 30000; // 30 секунд таймаут
    cout << "Ожидание доступности сервера..." << endl;

    if (!WaitNamedPipeA("\\\\.\\pipe\\MatrixInput", 5000)) {
        DWORD err = GetLastError();
        if (err == ERROR_SEM_TIMEOUT)
        {
            cerr << "Превышено время ожидания сервера" << endl;
        }
        else
        {
            cerr << "Ошибка подключения: " << err << endl;
        }
        _getch();
        return 1;
    }

    HANDLE hInputPipe = CreateFileA(
        "\\\\.\\pipe\\MatrixInput",
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hInputPipe == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        cerr << "Ошибка подключения: " << err << endl;
        _getch();
        return 1;
    }

    // Ввод матрицы
    Matrix matrix;
    SafeInput(matrix.rows, "Введите количество строк: ");
    SafeInput(matrix.cols, "Введите количество столбцов: ");
    
    matrix.data.resize(matrix.rows, vector<double>(matrix.cols));
    for (int i = 0; i < matrix.rows; ++i) {
        for (int j = 0; j < matrix.cols; ++j) {
            cout << "Элемент [" << (i + 1) << "][" << (j + 1) << "]: ";
            while (!(cin >> matrix.data[i][j])) {
                cin.clear();
                cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                cerr << "Некорректный ввод! Введите число: ";
            }
        }
    }

    // Отправка данных серверу

    if (!WriteMatrix(hInputPipe, matrix)) {
        cerr << "Ошибка передачи матрицы клиенту" << endl;
        _getch();
        return 1;
    }
    CloseHandle(hInputPipe);


    // Получение результата
    if (!WaitNamedPipeA("\\\\.\\pipe\\MatrixOutput", TIMEOUT)) {
        DWORD err = GetLastError();
        if (err == ERROR_SEM_TIMEOUT) {
            cerr << "Превышено время ожидания результата" << endl;
        } else {
            cerr << "Ошибка ожидания канала: " << err << endl;
        }
        _getch();
        return 1;
    }

    HANDLE hOutputPipe = CreateFileA(
        "\\\\.\\pipe\\MatrixOutput",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hOutputPipe == INVALID_HANDLE_VALUE) {
        cerr << "Ошибка подключения к выходному каналу: " << GetLastError() << endl;
        _getch();
        return 1;
    }

    Matrix result;
    if (!ReadMatrix(hOutputPipe, result)) {
        cerr << "Ошибка получения результата клиентом" << endl;
        _getch();
        return 1;
    }
    
    cout << "Результат сложения:" << endl;
    for (const auto& row : result.data) {
        for (double val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }

    CloseHandle(hOutputPipe);
    _getch();
    return 0;
}
