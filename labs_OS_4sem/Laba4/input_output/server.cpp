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

    // Создание именованных каналов
    HANDLE hInputPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\MatrixInput",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        8192, 8192,
        0,
        NULL);

    if (hInputPipe == INVALID_HANDLE_VALUE)
    {
        cerr << "Ошибка создания канала: " << GetLastError() << endl;
        _getch();
        return 1;
    }

    HANDLE hOutputPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\MatrixOutput",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        8192, 8192,
        0,
        NULL);

    if (hOutputPipe == INVALID_HANDLE_VALUE)
    {
        cerr << "Ошибка создания канала: " << GetLastError() << endl;
        _getch();
        return 1;
    }

    Matrix result; // Результирующая матрица
    bool isFirst = true;

    // Обработка подключений
    cout << "Ожидание подключения " << num_clients << " клиентов" << endl;

    for (int i = 0; i < num_clients; i++)
    {
        if (!ConnectNamedPipe(hInputPipe, NULL))
        {
            cerr << "Ошибка подключения клиента " << i << ": " << GetLastError() << endl;
            CloseHandle(hInputPipe);
            _getch();
            return 1;
        }
        cout << "Клиент " << i + 1 << " подключён" << endl;

        // Чтение матрицы
        Matrix matrix;
        if (!ReadMatrix(hInputPipe, matrix))
        {
            cerr << "Ошибка чтения данных клиента " << i + 1 << endl;
            DisconnectNamedPipe(hInputPipe);
            continue;
        }

        // Инициализация результирующей матрицы первой из суммы
        if (isFirst)
        {
            result = matrix;
            isFirst = false;
        }
        else
        {
            // Сумма матриц
            if (result.rows != matrix.rows || result.cols != matrix.cols)
            {
                cerr << "Несовпадение размеров матрицы!" << endl;
                _getch();
                return 1;
            }

            for (int row = 0; row < result.rows; row++)
            {
                for (int col = 0; col < result.cols; col++)
                {
                    result.data[row][col] += matrix.data[row][col];
                }
            }
        }

        DisconnectNamedPipe(hInputPipe);
    }
    CloseHandle(hInputPipe);
    
    for (int i = 1; i < num_clients; i++)
    {
        HANDLE hOutputPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\MatrixOutput",
            PIPE_ACCESS_OUTBOUND,
            PIPE_TYPE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            8192, 8192,
            0,
            NULL);
    
        if (hOutputPipe == INVALID_HANDLE_VALUE)
        {
            cerr << "Ошибка создания канала: " << GetLastError() << endl;
            _getch();
            return 1;
        }
        
        // Рассылка результата клиентам
        if (!ConnectNamedPipe(hOutputPipe, NULL))
        {
            cerr << "Ошибка соединения с клиентом " << i + 1 << " для передачи результата" << endl;
            CloseHandle(hOutputPipe);
            _getch();
            return 1;
        }
        if (!WriteMatrix(hOutputPipe, result))
        {
            cerr << "Ошибка передачи матрицы клиенту " << i + 1 << endl;
            DisconnectNamedPipe(hOutputPipe);
            CloseHandle(hOutputPipe);
            _getch();
            return 1;
        }

        cout << "Передача данных клиенту " << i + 1 << endl;

        DisconnectNamedPipe(hOutputPipe);
    }
    CloseHandle(hOutputPipe);

    cout << "Сервер завершил работу" << endl;
    _getch();
    return 0;
}
