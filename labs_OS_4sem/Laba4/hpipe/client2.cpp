#include <windows.h>
#include <iostream>
#include <vector>
#include <conio.h>
#include <limits>
#include <ios>
#include <string>

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
    if (!ReadFile(hPipe, &matrix.rows, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int))
        return false;
    if (!ReadFile(hPipe, &matrix.cols, sizeof(int), &bytesRead, NULL) || bytesRead != sizeof(int))
        return false;

    matrix.data.resize(matrix.rows, vector<double>(matrix.cols));
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
        for (auto val : row)
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
        if (cin.fail() || value <= 0)
        {
            cin.clear();
            cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            cerr << "Некорректный ввод! Повторите попытку" << endl;
            continue;
        }
        break;
    }
    cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Ожидание канала
    const DWORD TIMEOUT = 30000;
    cout << "Ожидание сервера..." << endl;
    if (!WaitNamedPipeA("\\\\.\\pipe\\MatrixPipe", TIMEOUT))
    {
        DWORD err = GetLastError();
        if (err == ERROR_SEM_TIMEOUT)
        {
            cerr << "Сервер не ответил." << endl;
        }
        else
        {
            cerr << "Ошибка: " << err << endl;
        }
        _getch();
        return 1;
    }

    // Подключение к каналу
    HANDLE hPipe = CreateFileA(
        "\\\\.\\pipe\\MatrixPipe",
        GENERIC_READ | GENERIC_WRITE, // Чтение и запись
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        cerr << "Ошибка подключения: " << GetLastError() << endl;
        _getch();
        return 1;
    }

    // Ввод матрицы
    Matrix matrix;
    SafeInput(matrix.rows, "Введите количество строк: ");
    SafeInput(matrix.cols, "Введите количество столбцов: ");

    matrix.data.resize(matrix.rows, vector<double>(matrix.cols));
    for (int i = 0; i < matrix.rows; ++i)
    {
        for (int j = 0; j < matrix.cols; ++j)
        {
            cout << "Элемент [" << i + 1 << "][" << j + 1 << "]: ";
            while (!(cin >> matrix.data[i][j]))
            {
                cin.clear();
                cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                cerr << "Ошибка! Введите число: ";
            }
        }
    }

    // Отправка матрицы
    if (!WriteMatrix(hPipe, matrix))
    {
        cerr << "Ошибка отправки данных." << endl;
        CloseHandle(hPipe);
        _getch();
        return 1;
    }

    // Получение результата
    Matrix result;
    if (!ReadMatrix(hPipe, result))
    {
        cerr << "Ошибка получения результата." << endl;
        CloseHandle(hPipe);
        _getch();
        return 1;
    }
    else
    {
        cout << "\nРезультат сложения:" << endl;
        for (const auto &row : result.data)
        {
            for (double val : row)
            {
                cout << val << "\t";
            }
            cout << endl;
        }
    }

    CloseHandle(hPipe);
    _getch();
    return 0;
}
