#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <limits>
#include <ios>
using namespace std;

// Вектор сокетов
vector<SOCKET> Connections;
int Counter = 0;

void ClientHandler(int index) {
    int msg_size;
    while (true) {
        recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
        char* msg = new char[msg_size + 1];
        msg[msg_size] = '\0';
        recv(Connections[index], msg, sizeof(msg), NULL);
        for (int i = 0; i < Counter; i++) {
            if (i == index) continue;

            send(Connections[i], (char*)&msg_size, sizeof(int), NULL);
            send(Connections[i], msg, msg_size, NULL);
        }
        delete[] msg;
    }
}

void SafeInputNumClients(int &value, const string &prompt)
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

        if (value < 1)
        {
            cerr << "Значение должно быть >= 1" << endl;
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

    // Количество клиентов
    int num_clients;
    SafeInputNumClients(num_clients, "Введите количество клиентов (>= 1): ");

    // WSAStartup
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1); // Запрашиваемая версия библиотеки WinSock
    if (WSAStartup(DLLVersion, &wsaData) != 0) // Загрузка библиотеки
    { 
        cerr << "Ошибка WSAStartup" << endl;
        return 1;
    }

    SOCKADDR_IN addr; // Хранение адреса для интернет-протокола
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // sin_addr хранит ip-адрес (здесь - localhost)
    addr.sin_port = htons(1111);                   // Установка порта
    addr.sin_family = AF_INET;                     // Семейство протоколов (константа AF_INET для интернет-протоколов)

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL); // Создание сокета для прослушивания
    bind(sListen, (SOCKADDR *)&addr, sizeof(addr));   // Привязка адреса сокету
    // Запуск прослушивания
    listen(sListen,
           SOMAXCONN // Максимально допустимое число запросов, допускающих обработки
    );

    SOCKET newConnection; // Сокет для удержания соединения с клиентом
    for (int i = 0; i < num_clients; i++)
    {
        newConnection = accept(sListen, (SOCKADDR *)&addr, &sizeofaddr);

        // Проверка подключения клиента к серверу
        if (newConnection == 0)
        {
            cerr << "Ошибка, клиент не может подключиться к серверу" << endl;
        }
        else
        {
            cout << "Клиент подключён!" << endl;
            string msg = "Здарова, перец";
            int msg_size = sizeof(msg);
            send(newConnection, (char*)&msg_size, sizeof(int), NULL);
            send(newConnection, msg.c_str(), msg_size, NULL);

            Connections[i] = newConnection;
            ++Counter;
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
        }
    }

    system("pause");
    return 0;
}
