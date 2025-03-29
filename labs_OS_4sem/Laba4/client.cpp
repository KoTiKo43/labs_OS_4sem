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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Использование: client.exe <ID_клиента>" << endl;
        return 1;
    }
    int clientId = atoi(argv[1]);
    string pipeName = "\\\\.\\pipe\\MatrixPipe_" + to_string(clientId);

    // Отправка матрицы
    cout << "Подключение к каналу " << pipeName << "..." << endl;
    if (!WaitNamedPipeA(pipeName.c_str(), NMPWAIT_WAIT_FOREVER)) {
        cerr << "Ошибка ожидания канала: " << GetLastError() << endl;
        return 1;
    }
    HANDLE hPipe = CreateFileA(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (hPipe == INVALID_HANDLE_VALUE) {
        cerr << "Ошибка открытия канала: " << GetLastError() << endl;
        return 1;
    }

    // Ввод данных
    Matrix matrix;
    cout << "Введите количество строк: ";
    cin >> matrix.rows;
    cout << "Введите количество столбцов: ";
    cin >> matrix.cols;
    matrix.data.resize(matrix.rows * matrix.cols);
    cout << "Введите элементы матрицы:" << endl;
    for (int i = 0; i < matrix.rows * matrix.cols; ++i) {
        cin >> matrix.data[i];
    }

    // Отправка
    DWORD bytesWritten;
    if (!WriteFile(hPipe, &matrix.rows, sizeof(matrix.rows), &bytesWritten, NULL)) {
        cerr << "Ошибка записи строк: " << GetLastError() << endl;
        CloseHandle(hPipe);
        return 1;
    }
    if (!WriteFile(hPipe, &matrix.cols, sizeof(matrix.cols), &bytesWritten, NULL)) {
        cerr << "Ошибка записи столбцов: " << GetLastError() << endl;
        CloseHandle(hPipe);
        return 1;
    }
    if (!WriteFile(hPipe, matrix.data.data(), matrix.rows * matrix.cols * sizeof(int), &bytesWritten, NULL)) {
        cerr << "Ошибка записи данных: " << GetLastError() << endl;
        CloseHandle(hPipe);
        return 1;
    }
    cout << "Матрица отправлена." << endl;
    CloseHandle(hPipe);

    // Получение результата
    cout << "Ожидание результата..." << endl;
    if (!WaitNamedPipeA(pipeName.c_str(), NMPWAIT_WAIT_FOREVER)) {
        cerr << "Ошибка ожидания канала: " << GetLastError() << endl;
        return 1;
    }
    HANDLE hPipeResult = CreateFileA(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (hPipeResult == INVALID_HANDLE_VALUE) {
        cerr << "Ошибка открытия канала: " << GetLastError() << endl;
        return 1;
    }

    // Чтение результата
    Matrix result;
    DWORD bytesRead;
    if (!ReadFile(hPipeResult, &result.rows, sizeof(result.rows), &bytesRead, NULL) || bytesRead != sizeof(result.rows)) {
        cerr << "Ошибка чтения строк: " << GetLastError() << endl;
        CloseHandle(hPipeResult);
        return 1;
    }
    if (!ReadFile(hPipeResult, &result.cols, sizeof(result.cols), &bytesRead, NULL) || bytesRead != sizeof(result.cols)) {
        cerr << "Ошибка чтения столбцов: " << GetLastError() << endl;
        CloseHandle(hPipeResult);
        return 1;
    }
    result.data.resize(result.rows * result.cols);
    if (!ReadFile(hPipeResult, result.data.data(), result.rows * result.cols * sizeof(int), &bytesRead, NULL) || bytesRead != result.rows * result.cols * sizeof(int)) {
        cerr << "Ошибка чтения данных: " << GetLastError() << endl;
        CloseHandle(hPipeResult);
        return 1;
    }
    CloseHandle(hPipeResult);

    // Вывод результата
    cout << "Результат суммирования:" << endl;
    for (int i = 0; i < result.rows; ++i) {
        for (int j = 0; j < result.cols; ++j) {
            cout << result.data[i * result.cols + j] << " ";
        }
        cout << endl;
    }

    return 0;
}
