#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <conio.h>
#include <limits>
#include <ios>

using namespace std;

struct Matrix
{
    int rows;
    int cols;
    vector<vector<double>> data;
};

bool ReadMatrix(HANDLE hPipe, Matrix &matrix)
{
    DWORD bytesRead;

    // Чтение размеров матрицы
    if (!ReadFile(hPipe, &matrix.rows, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int))
        return false;
    if (!ReadFile(hPipe, &matrix.cols, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int))
        return false;

    // Инициализация матрицы
    matrix.data.resize(matrix.rows, vector<double>(matrix.cols));

    // Чтение элементов
    for (auto &row : matrix.data)
    {
        for (auto &val : row)
        {
            if (!ReadFile(hPipe, &val, sizeof(double), &bytesRead, NULL) || bytesRead != sizeof(double))
                return false;
        }
    }
    return true;
}

bool WriteMatrix(HANDLE hPipe, const Matrix &matrix)
{
    DWORD bytesWritten;

    if (!WriteFile(hPipe, &matrix.rows, sizeof(int), &bytesWritten, NULL) || bytesWritten != sizeof(int))
        return false;
    if (!WriteFile(hPipe, &matrix.cols, sizeof(int), &bytesWritten, NULL) || bytesWritten != sizeof(int))
        return false;

    for (const auto &row : matrix.data)
    {
        for (auto &val : row)
        {
            if (!WriteFile(hPipe, &val, sizeof(double), &bytesWritten, NULL))
                return false;
        }
    }
    return true;
}

void SafeInput(int &value, const string &prompt)
{
    while (true)
    {
        cout << prompt;
        cin >> value;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            cerr << "Ошибка ввода! Введите число" << endl;
            continue;
        }

        if (value < 2)
        {
            cerr << "Значение должно быть >= 2" << endl;
            continue;
        }
        break;
    }
    cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

int main()
{
    // Установка кодировки вывода консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Считывание количества клиентов
    int num_clients;
    SafeInput(num_clients, "Введите количество клиентов (>= 2): ");

    // Создание двустороннего канала
    HANDLE hPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\MatrixPipe",
        PIPE_ACCESS_DUPLEX,  // Чтение и запись
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        num_clients,  // Максимум клиентов
        8192, 8192, 0, NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cerr << "Ошибка создания канала: " << GetLastError() << endl;
        _getch();
        return 1;
    }

    Matrix result;
    bool isFirst = true;

    for (int i = 0; i < num_clients; ++i) {
        cout << "Ожидание клиента " << i+1 << "..." << endl;
        if (!ConnectNamedPipe(hPipe, NULL)) {
            cerr << "Ошибка подключения: " << GetLastError() << endl;
            CloseHandle(hPipe);
            _getch();
            return 1;
        }

        // Чтение матрицы от клиента
        Matrix matrix;
        if (!ReadMatrix(hPipe, matrix)) {
            cerr << "Ошибка чтения данных." << endl;
            DisconnectNamedPipe(hPipe);
            continue;
        }

        // Суммирование
        if (isFirst) {
            result = matrix;
            isFirst = false;
        } else {
            if (result.rows != matrix.rows || result.cols != matrix.cols) {
                cerr << "Несовпадение размеров матрицы!" << endl;
                DisconnectNamedPipe(hPipe);
                _getch();
                return 1;
            }
            for (int row = 0; row < result.rows; ++row) {
                for (int col = 0; col < result.cols; ++col) {
                    result.data[row][col] += matrix.data[row][col];
                }
            }
        }

        // Отправка результата обратно клиенту
        if (!WriteMatrix(hPipe, result)) {
            cerr << "Ошибка отправки результата." << endl;
        }

        cout << "Результаты отправлены клиенту " << i + 1 << endl;

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
    cout << "Сервер завершил работу." << endl;
    _getch();
    return 0;
}
