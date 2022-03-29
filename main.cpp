#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <numeric>
#include <thread>

#define HOST_IP "127.0.0.1"
#define OPENSIGNALPORT 5555
#define UNREALPORT 3457

float   calm = 11;
pthread_mutex_t focus_mutex;
int     right = 0;
int     left = 0;
bool    getEyes = false;
bool    getMeditation = false;
int     client;
std::vector<float>      values;
std::vector<float>      variations;
std::vector<float>      focus;
std::string         res;

void    error(std::string str) {
    std::cout << str << std::endl;
    exit(1);
}

void    getLook() {
    if (values.back() > 20) { //first of right or second of left
        if (left > 0) {
            res = "22e09328392323423432423423";
            return ;
        }
        else
            right = 3;
    }
    else if (values.back() < -20) { //first of left or second of right
        if (right > 0) {
            res = "1";
            return ;
        }
        else
            left = 3;
    }
    else if (values.back() < 20 && values.back() > -20) {
        right--;
        left--;
    }
}

void    getAverage() {
    usleep(400000);
    if (variations.size() > 20) {
        std::vector<float> tmp;
        for (int i = variations.size() - 1; i > 20; i--) {
            if (variations[i] > 0)
                tmp.push_back(variations[i]);
        }
        int sum = 0;
        for (int i = 0; i < 20; i++)
            sum += variations[i];
        float average = sum / 20;
        focus.push_back(average);
        tmp.clear();
    }
}

void   getFocus() {
    usleep(500000);
    if (focus.size() > 10) {
        std::vector<float> tmp(focus.end() - 5, focus.end());
        float average = std::accumulate(tmp.begin(), tmp.end(), 0.0f) / tmp.size();
        if (average < calm) {
            res = "1";
            getMeditation = false;
            return ;
        }
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
        if (values.size() > 1) {
            float var = values[values.size() - 1] - values[values.size() - 2];
            variations.push_back(abs(var));
        }
        if (getEyes == true)
            getLook();
        else if (getMeditation == true) {
            getAverage();
            getFocus();
        }
        str.erase(0, secondBracket + 2);
    }
}

void    *sendRes(void *) {
    while (1) {
        if (res.size() > 0) {
            send(client, res.c_str(), res.length(), 0);
            std::cout << "sent: " << res << std::endl;
            pthread_mutex_lock(&focus_mutex);
            res.clear();
            pthread_mutex_unlock(&focus_mutex);
            getEyes = false;
            getMeditation = false;
        }
    }
    return (NULL);
}

void    *getRequest(void *) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in     addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(UNREALPORT);
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        std::cout << "errno: " << errno << std::endl;
        error("Binding failed");
    }
    listen(sock, 1000);
    int connected = 1;
    client = accept(sock, NULL, NULL);
    std::cout << "client accepted on socket " << client << std::endl;    
    while (1) {
        if (connected == 0) {
            client = accept(sock, NULL, NULL);
            std::cout << "client accepted on socket " << client << std::endl;
        }
        send(client, "0", 1, 0);
        char buffer[255];
        if (recv(client, buffer, sizeof(buffer), 0) == 0) {
            connected = 0;
            continue;
        }
        if (strlen(buffer) > 0)
            std::cout << "received " << buffer << std::endl;
        std::string input = buffer;
        std::cout << input << std::endl;
        if (input.find("FOCUS") != std::string::npos) {
            getMeditation = true;
        }
        else if (input.find("EYES") != std::string::npos) {
            getEyes = true;
        }
        memset(buffer, '\0', 255);
    }
    close(sock);
}

int     main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    struct sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OPENSIGNALPORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    send(sock, "start", 5, 0);
    variations.push_back(0.0);
    pthread_t   unrealId;
    pthread_t id;
    pthread_mutex_init(&focus_mutex, NULL);
    pthread_create(&unrealId, NULL, getRequest, NULL);
    pthread_create(&id, NULL, sendRes, NULL);
    while (1) {
        char buffer[1000];
        recv(sock, buffer, sizeof(buffer), 0);
        parsing(buffer);
        memset(buffer, '\0', 1000);
        if (values.size() > 100)
            values.erase(values.begin(), values.begin() + 50);
        if (variations.size() > 100)
            variations.erase(variations.begin(), variations.begin() + 50);
        if (focus.size() > 100)
            focus.erase(focus.begin(), focus.begin() + 50);        
    }
    pthread_join(unrealId, NULL);
    pthread_join(id, NULL);
    pthread_mutex_destroy(&focus_mutex);
    close(sock);
    return (0);
}