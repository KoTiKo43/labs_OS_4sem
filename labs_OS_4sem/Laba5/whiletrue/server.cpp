#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <limits>
#include <ios>
#include <string>
#include <map>
#include <sstream>
using namespace std;

// Вектор сокетов
vector<SOCKET> Connections;

// Создание семафора
HANDLE hSemaphore;
HANDLE hSemaphoreConnect;

char decodeMorse(const string &ch)
{
    static map<string, char> morseDecode = {
        {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'}, 
        {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'}, 
        {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, 
        {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'}, 
        {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'}, 
        {"--..", 'Z'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, 
        {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}, 
        {"-----", '0'}
    };

    if (morseDecode.find(ch) != morseDecode.end())
    {
        return morseDecode[ch];
    }
    else
    {
        return '?';
    }
}

string decodeMorseMsg(const char *msg)
{
    stringstream ss(msg);
    string ch;
    string result = "";
    while (ss >> ch)
    {
        if (ch == "/")
        {
            result += "";
        }
        else if (ch == "#")
        {
            result += " ";
        }
        else
        {
            result += decodeMorse(ch);
        }
    }
    return result;
}

void ClientHandler(int index)
{
    while (true)
    {
        // Ожидание семафора
        WaitForSingleObject(hSemaphore, INFINITE);

        int msg_size;

        // Обработка отключения клиента
        if (recv(Connections[index], (char *)&msg_size, sizeof(int), NULL) <= 0)
        {
            cout << "Юстас №" << index + 1 << " прервал связь" << endl;
            closesocket(Connections[index]);
            ReleaseSemaphore(hSemaphore, 1, NULL);
            break;
        }

        char *msg = new char[msg_size + 1];
        msg[msg_size] = '\0';
        recv(Connections[index], msg, msg_size, NULL);

        if (string(msg) == "_END")
        {
            cout << "Юстас №" << index + 1 << " прервал связь" << endl;
            closesocket(Connections[index]);
            delete[] msg;
            ReleaseSemaphore(hSemaphore, 1, NULL);
            break;
        }

        cout << "Шифровка от Юстаса №" << index + 1 << ": " << msg << endl;
        cout << "Дешифровка: " << decodeMorseMsg(msg) << endl;

        delete[] msg;

        // Освобождение семафора обработки сообщений
        ReleaseSemaphore(hSemaphore, 1, NULL);
    }

    // Освобождение семафора подключений
    ReleaseSemaphore(hSemaphoreConnect, 1, NULL);
}

int main()
{
    // Установка кодировки вывода консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // WSAStartup
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);          // Запрашиваемая версия библиотеки WinSock
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
    bind(sListen, (SOCKADDR *)&addr, sizeof(addr));      // Привязка адреса сокету
    listen(sListen, SOMAXCONN);                          // Запуск прослушивания

    // Создание семафора при n = 2
    const int n = 2;
    hSemaphore = CreateSemaphore(NULL, n, n, NULL);
    const int maxConnections = 2;
    hSemaphoreConnect = CreateSemaphore(NULL, maxConnections, maxConnections, NULL);

    SOCKET newConnection; // Сокет для удержания соединения с клиентом
    cout << "Связь настроена. Ожидание подключений..." << endl;

    while (true)
    {
        WaitForSingleObject(hSemaphoreConnect, INFINITE);

        newConnection = accept(sListen, (SOCKADDR *)&addr, &sizeofaddr);

        // Проверка подключения клиента к серверу
        if (newConnection == 0)
        {
            cerr << "Ошибка, клиент не может подключиться к серверу" << endl;
        }
        else
        {
            Connections.push_back(newConnection);
            int clientNumber = Connections.size(); // Номер клиента
            cout << "Юстас №" << clientNumber << " на связи!" << endl;

            // Отправляем номер клиенту
            send(newConnection, (char *)&clientNumber, sizeof(int), NULL);
            cout << "Отправку Юстасу его номера " << clientNumber << endl;

            // Запускаем поток для обработки клиента
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(Connections.size() - 1), NULL, NULL);
        }
    }

    // Закрытие семафора
    CloseHandle(hSemaphore);
    CloseHandle(hSemaphoreConnect);

    for (SOCKET sock : Connections)
    {
        closesocket(sock);
    }
    closesocket(sListen);
    WSACleanup();
    system("pause");
    return 0;
}
