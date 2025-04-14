#include <iostream>
#include <WinSock2.h>
#include <string>
#include <map>
using namespace std;

SOCKET Connection;

// string encodeToMorse(const string& text) {
//     static map<char, string> morseCode = {
//         {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
//         {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
//         {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
//         {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
//         {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
//         {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
//         {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"},
//     };
//     string result;
//     for (char c : text) {
//         if (c == ' ') {
//             result += " / ";
//         } else {
//             char upperC = toupper(c);
//             if (morseCode.find(upperC) != morseCode.end()) {
//                 result += morseCode[upperC];
//             }
//         }
//     }
//     return result;
// }

int main()
{
    // Установка кодировки вывода консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // WSAStartup
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
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
    if (connect(Connection, (SOCKADDR *)&addr, sizeof(addr)) != 0)
    {
        cerr << "Ошибка, не удалось подключиться к серверу" << endl;
        return 1;
    }
    // Получение номера от сервера
    int clientNumber;
    recv(Connection, (char *)&clientNumber, sizeof(int), NULL);

    cout << "Связь установлена. Здравствуйте, Юстас №" << clientNumber << endl;

    // Отправка строки серверу
    string msg;
    while (true)
    {
        getline(cin, msg);
        int msg_size = msg.size();
        send(Connection, (char *)&msg_size, sizeof(int), NULL);
        send(Connection, msg.c_str(), msg_size, NULL);
        Sleep(10);
    }

    closesocket(Connection);
    WSACleanup();
    system("pause");
    return 0;
}
