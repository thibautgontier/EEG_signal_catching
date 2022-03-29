#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <numeric>

#define HOST_IP "127.0.0.1"
#define OPENSIGNALPORT 5555
#define UNREALPORT 7000

float   calm = 7.0;
int     right;
int     left;
std::vector<float>      values;
std::vector<float>      variations;
std::vector<float>      focus;
std::string         res;


void    error(std::string str) {
    std::cout << str << std::endl;
    exit(1);
}

void    getLook() {
    if (values.back() > 25) { //first of right or second of left
        if (left > 0) {
            std::cout << "left\n";
            return ;
        }
        else
            right = 3;
    }
    else if (values.back() < -25) { //first of left or second of right
        if (right > 0) {
            std::cout << "right\n";
            return ;
        }
        else
            left = 3;
    }
    else if (values.back() < 25 && values.back() > -25) {
        right--;
        left--;
    }
}

void    parsing(char *buffer) {
    std::string str = buffer;
    int bigBracket = 54;
    str.erase(0, 55);
    while (str.find("[") != std::string::npos) {
        std::string secondNumber;
        int secondBracket = str.find("]");
        int i = secondBracket;
        while (str[i] != ' ')
            i--;
        while (++i != secondBracket)
            secondNumber += str[i];
        float num = atof(secondNumber.c_str());
        values.push_back(num);
        float var = values[values.size() - 1] - values[values.size() - 2];
        variations.push_back(abs(var));
        getLook();
        str.erase(0, secondBracket + 2);
    }
}

int     main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OPENSIGNALPORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    send(sock, "start", 5, 0);
    pthread_t varId;
    pthread_t focusId;
    pthread_t lookId;
    right = 0;
    left = 0;
    variations.push_back(0.0);
    pthread_t   unrealId;
    while (1) {
        char buffer[1000];
        int res = recv(sock, buffer, sizeof(buffer), 0);
        parsing(buffer);
        memset(buffer, '\0', 1000);
    }
    pthread_join(varId, NULL);
    pthread_join(focusId, NULL);
    pthread_join(unrealId, NULL);
    close(sock);
    return (0);
}