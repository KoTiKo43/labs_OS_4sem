#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

struct Matrix {
    int rows;
    int cols;
    vector<int> data;
};

int main() {
    const int n = 2; 
    vector<HANDLE> pipes;

    // Создание каналов
    for (int i = 0; i < n; ++i) {
        string pipeName = "\\\\.\\pipe\\MatrixPipe_" + to_string(i);
        HANDLE hPipe = CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 
            1024, 1024, 
            0, 
            NULL
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            cerr << "Ошибка создания канала: " << GetLastError() << endl;
            return 1;
        }
        pipes.push_back(hPipe);
        cout << "Создан канал: " << pipeName << endl;
    }

    // Запуск клиентов
    for (int i = 0; i < n; ++i) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        string cmd = "client.exe " + to_string(i);
        if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            cerr << "Ошибка запуска клиента: " << GetLastError() << endl;
            return 1;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        cout << "Запущен клиент " << i << endl;
    }

    // Получение матриц
    vector<Matrix> matrices;
    for (int i = 0; i < n; ++i) {
        HANDLE hPipe = pipes[i];
        cout << "Ожидание подключения клиента " << i << "..." << endl;
        if (!ConnectNamedPipe(hPipe, NULL)) {
            cerr << "Ошибка подключения: " << GetLastError() << endl;
            return 1;
        }
        cout << "Клиент " << i << " подключен." << endl;

        Matrix matrix;
        DWORD bytesRead;
        if (!ReadFile(hPipe, &matrix.rows, sizeof(matrix.rows), &bytesRead, NULL) || bytesRead != sizeof(matrix.rows)) {
            cerr << "Ошибка чтения строк: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        if (!ReadFile(hPipe, &matrix.cols, sizeof(matrix.cols), &bytesRead, NULL) || bytesRead != sizeof(matrix.cols)) {
            cerr << "Ошибка чтения столбцов: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        matrix.data.resize(matrix.rows * matrix.cols);
        if (!ReadFile(hPipe, matrix.data.data(), matrix.rows * matrix.cols * sizeof(int), &bytesRead, NULL) || bytesRead != matrix.rows * matrix.cols * sizeof(int)) {
            cerr << "Ошибка чтения данных: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        matrices.push_back(matrix);
        cout << "Получена матрица от клиента " << i << endl;
        DisconnectNamedPipe(hPipe);
    }

    // Проверка размеров
    int rows = matrices[0].rows;
    int cols = matrices[0].cols;
    for (const auto& matrix : matrices) {
        if (matrix.rows != rows || matrix.cols != cols) {
            cerr << "Матрицы разного размера!" << endl;
            return 1;
        }
    }

    // Суммирование
    Matrix sum;
    sum.rows = rows;
    sum.cols = cols;
    sum.data.resize(rows * cols, 0);
    for (const auto& matrix : matrices) {
        for (int i = 0; i < rows * cols; ++i) {
            sum.data[i] += matrix.data[i];
        }
    }

    // Отправка результата
    for (int i = 0; i < n; ++i) {
        HANDLE hPipe = pipes[i];
        cout << "Ожидание подключения клиента " << i << " для результата..." << endl;
        if (!ConnectNamedPipe(hPipe, NULL)) {
            cerr << "Ошибка подключения: " << GetLastError() << endl;
            return 1;
        }
        cout << "Клиент " << i << " подключен для получения результата." << endl;

        DWORD bytesWritten;
        if (!WriteFile(hPipe, &sum.rows, sizeof(sum.rows), &bytesWritten, NULL) || bytesWritten != sizeof(sum.rows)) {
            cerr << "Ошибка записи строк: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        if (!WriteFile(hPipe, &sum.cols, sizeof(sum.cols), &bytesWritten, NULL) || bytesWritten != sizeof(sum.cols)) {
            cerr << "Ошибка записи столбцов: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        if (!WriteFile(hPipe, sum.data.data(), sum.rows * sum.cols * sizeof(int), &bytesWritten, NULL) || bytesWritten != sum.rows * sum.cols * sizeof(int)) {
            cerr << "Ошибка записи данных: " << GetLastError() << endl;
            DisconnectNamedPipe(hPipe);
            return 1;
        }
        cout << "Результат отправлен клиенту " << i << endl;
        DisconnectNamedPipe(hPipe);
    }

    // Закрытие каналов
    for (auto& hPipe : pipes) {
        CloseHandle(hPipe);
    }

    return 0;
}
