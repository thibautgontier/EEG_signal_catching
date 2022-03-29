#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <numeric>

#define HOST_IP "127.0.0.1"
#define PORT 5555

float   calm = 6.0;
std::vector<float>      values;
std::vector<float>      variations;
std::vector<float>      focus;

void    parsing(char *buffer) {
    std::string str = buffer;
    int bigBracket = 54;
    str.erase(0, 55);
    variations.push_back(0.0);
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
        str.erase(0, secondBracket + 2);
    }
}

void    *getAverage(void *) {
    while (1) {
        usleep(400000);
        if (variations.size() > 20) {
            std::vector<float> tmp(variations.end() - 20, variations.end());
            float average = std::accumulate(tmp.begin(), tmp.end(), 0.0) / tmp.size();
            focus.push_back(average);
            tmp.clear();
        }
    }
    return (NULL);
}

void    *getFocus(void *) {
    while (1) {
        usleep(500000);
        if (focus.size() > 4) {
            std::vector<float> tmp(focus.end() - 4, focus.end());
            float average = std::accumulate(tmp.begin(), tmp.end(), 0.0) / tmp.size();
            std::string focused = (average < calm) ? "calm" : "not calm";
            std::cout << "You are: " << focused << std::endl;
        }
    }
}

void    error(std::string str) {
    std::cout << str << std::endl;
    exit(1);
}

int     main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    send(sock, "start", 5, 0);
    pthread_t var_id;
    pthread_t focus_id;
    pthread_create(&var_id, NULL, getAverage, NULL);
    pthread_create(&focus_id, NULL, getFocus, NULL);
    while (1) {
        char buffer[1000];
        int res = recv(sock, buffer, sizeof(buffer), 0);
        parsing(buffer);
        memset(buffer, '\0', 1000);
    }
    pthread_join(var_id, NULL);
    pthread_join(focus_id, NULL);
    close(sock);
    return (0);
}