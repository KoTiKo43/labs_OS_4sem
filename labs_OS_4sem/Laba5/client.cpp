#include <iostream>
#include <WinSock2.h>
#include <string>
using namespace std;

SOCKET Connection;

void ClientHandler() {
    int msg_size;
    while(true) {
        recv(Connection, (char*)&msg_size, sizeof(int), NULL);
        char* msg = new char[msg_size + 1];
        msg[msg_size] = '\0';
        recv(Connection, msg, msg_size, NULL);
        cout << msg << endl;
        delete[] msg;
    }
}

int main() {
    // Установка кодировки вывода консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // WSAStartup
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        cerr << "Ошибка WSAStartup" << endl;
        return 1;
    }

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    Connection = socket(AF_INET, SOCK_STREAM, 0); // Сокет для соединения с сервером
    // Попытка соединения с сервером
    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
        cerr << "Ошибка, не удалось подключиться к серверу" << endl;
        return 1;
    }
    // Получение номера от сервера
    int clientNumber;
    recv(Connection, (char*)&clientNumber, sizeof(int), NULL);

    cout << "Связь установлена. Ваш номер: " << clientNumber << endl;

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

    // Отправка строки серверу
    string msg1;
    while(true) {
        getline(cin, msg1);
        int msg_size = msg1.size();
        send(Connection, (char*)&msg_size, sizeof(int), NULL);
        send(Connection, msg1.c_str(), msg_size, NULL);
        Sleep(10);
    }

    system("pause");
    return 0;
}
