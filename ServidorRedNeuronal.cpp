#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "tictactoe.h"

struct ClientConfig {
    std::vector<std::string> nombres;
    std::vector<int> ips;
    std::vector<bool> elegido;
};

std::multimap<int, std::string> DataAprendisaje;
std::vector<std::vector<double>> boards;
std::vector<int> nextMoves;
std::map<double,vector<double>> target;
std::vector<double> result;
std::vector<double> bestPlay;
std::vector<double> sftmxResult;
std::vector<double> sftmxBestPlay;
Perceptron MLP=buildPerceptron();

ClientConfig client_config;
std::vector<int> received_keys;
std::vector<std::string> received_values;
bool keep_alive_active = false;

void handle_tcp_connection(int tcp_socket);
void handle_udp_connection(int udp_socket, sockaddr_in udp_server_addr);
void print_client_config();
void send_keep_alive(int tcp_socket);
bool compare_indices(int a, int b) {
    return sftmxBestPlay[a] > sftmxBestPlay[b]; // Comparación para ordenar según los valores en vec
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <udp_ip> <udp_port>" << std::endl;
        return 1;
    }

    const char* udp_ip = argv[1];
    int udp_port = std::stoi(argv[2]);

    // Setup TCP client
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in tcp_server_addr;
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tcp_server_addr.sin_port = htons(8080);

    connect(tcp_socket, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr));

    // Setup UDP client
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udp_client_addr;
    udp_client_addr.sin_family = AF_INET;
    udp_client_addr.sin_addr.s_addr = inet_addr(udp_ip);
    udp_client_addr.sin_port = htons(udp_port);

    bind(udp_socket, (struct sockaddr *)&udp_client_addr, sizeof(udp_client_addr));

    sockaddr_in udp_server_addr;
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    udp_server_addr.sin_port = htons(8081);

    // Send autologin message over TCP
    std::string tcp_message = "1";
    send(tcp_socket, tcp_message.c_str(), tcp_message.size(), 0);

    // Send autologin message over UDP
    std::string udp_message = "1";
    sendto(udp_socket, udp_message.c_str(), udp_message.size(), 0, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr));

    std::thread tcp_thread(handle_tcp_connection, tcp_socket);
    std::thread udp_thread(handle_udp_connection, udp_socket, udp_server_addr);

    tcp_thread.join();
    udp_thread.join();

    close(tcp_socket);
    close(udp_socket);

    return 0;
}

void handle_tcp_connection(int tcp_socket) {
    char buffer[1024];
    while (true) {
        int bytes_received = read(tcp_socket, buffer, sizeof(buffer));
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            std::istringstream ss(message.substr(1));
            std::string token;
            switch (message[0]) {
                case '1': {
                    client_config.nombres.clear();
                    while (std::getline(ss, token, ',')) {
                        client_config.nombres.push_back(token);
                    }
                    std::cout << "Updated names: ";
                    for (const auto& name : client_config.nombres) {
                        std::cout << name << " ";
                    }
                    std::cout << std::endl;
                    break;
                }
                case '2': {
                    client_config.ips.clear();
                    while (std::getline(ss, token, ',')) {
                        try {
                            client_config.ips.push_back(std::stoi(token));
                        } catch (const std::invalid_argument &e) {
                            std::cerr << "Invalid IP token: " << token << std::endl;
                        }
                    }
                    std::cout << "Updated IPs: ";
                    for (const auto& ip : client_config.ips) {
                        std::cout << ip << " ";
                    }
                    std::cout << std::endl;
                    break;
                }
                case '3': {
                    client_config.elegido.clear();
                    while (std::getline(ss, token, ',')) {
                        client_config.elegido.push_back(token == "1");
                    }
                    std::cout << "Updated selected: ";
                    for (const auto& selected : client_config.elegido) {
                        std::cout << selected << " ";
                    }
                    std::cout << std::endl;
                    break;
                }
                case 'H': {
                    if (!keep_alive_active) {
                        keep_alive_active = true;
                        std::thread(send_keep_alive, tcp_socket).detach();
                    }
                    break;
                }
                case 'G': { // Iniciar el entrenamiento desde el Terminal
                    processDataAprendisaje(DataAprendisaje,boards,nextMoves,target);
                    int generacion=0;
                    while(generacion<100){
                        // calcular resultado
                        cout<<"Generacion "<<++generacion<<endl;
                        for(unsigned i=0; i<boards.size(); ++i){
                        std::vector<double> result;
                        // cout<<"Numero: "<<nextMoves[i]<<endl;
                        MLP.feedForward(boards[i]);
                        MLP.getResults(result); // vector<double> result | resultado del entrenamiento
                        sftmxResult = MLP.softmax(result); // cambiar el result con el softmax | sftmx es global
                        // enviar sftmx y numero de indice (unsigned i del for) a las demas RNs
                        }
                    }
                }
                case 'C': { // Continuar el entrenamiento desde el Terminal
                    int generacion=0;
                    while(generacion<100){
                        // calcular resultado
                        std::cout<<"Generacion "<<++generacion<<std::endl;
                        for(unsigned i=0; i<boards.size(); ++i){
                        std::vector<double> result;
                        // cout<<"Numero: "<<nextMoves[i]<<endl;
                        MLP.feedForward(boards[i]);
                        MLP.getResults(result); // vector<double> result | resultado del entrenamiento
                        sftmxResult = MLP.softmax(result); // cambiar el result con el softmaxResult | sftmxResult es global
                        // enviar sftmx y numero de indice (unsigned i del for) a las demas RNs
                        }
                    }
                }
                case 'T': { // Recibir el tablero de la partida | Ej:T111111111000000000000000000

                    // Juego | T___111111111000000000000000000_
                    // T - idJugador de 3 bytes - tableroParsed - ficha

                    std::string tablero;
                    int bestMove=0;
                    // cout<<"Tablero:"<< tablero;
                    std::vector<double> board=stringToVector(tablero);
                    MLP.feedForward(board);
                    MLP.getResults(bestPlay);
                    sftmxBestPlay = MLP.softmax(bestPlay);

                    // showVector(" Resultado: ",result);
                    // showVector(" SoftMax: ", sftmx);

                    std::vector<int> bestPlayIndex(9);
                    for (int i = 0; i < 9; ++i) {
                        bestPlayIndex[i] = i;
                    }

                    std::sort(bestPlayIndex.begin(), bestPlayIndex.end(), compare_indices);

                    for(int play=0; play<9; ++play){
                        if(tablero[bestPlayIndex[play]]!=0){
                            bestMove=bestPlayIndex[play];
                            break;
                        }
                    }
                    std::cout<<"Mejor Jugada: "<<bestMove<<std::endl;

                    // enviar al Main la posicion
                    // T - idJugador - posicion
                }
                case 'M': { // Recibir la media del resultado de cada entrenamiento
                   // recibir sftmxResult y numero de indice
                    // actualizar pesos
                    int i;
                    if(MLP.checkError2(sftmxResult,target[nextMoves[i]])){
                        MLP.backProp(target[nextMoves[i]]);
                    }
                    // showVector(" Resultado: ", sftmx);
                    // showVector("  Esperado: ", target[nextMoves[i]]);
                    // cin.ignore();
                    MLP.saveWeights();
                    cout<<"Trained!"<<endl; 
                }
                case 'm': { // Recibir la media de la mejor jugada
                    // Calculo de la media y enviar al main que enviara al cliente
                }

                default:
                    std::cout << "Received TCP message: " << message << std::endl;
                    break;
            }
        }
    }
}

void handle_udp_connection(int udp_socket, sockaddr_in udp_server_addr) {
    while (true) {
        char buffer[1024];
        socklen_t server_addr_len = sizeof(udp_server_addr);
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&udp_server_addr, &server_addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            std::istringstream ss(message.substr(1));
            std::string token;
            switch (message[0]) {
                case 'K': {
                    received_keys.clear();
                    while (std::getline(ss, token, ',')) {
                        if (!token.empty()) {
                            received_keys.push_back(std::stoi(token));
                        }
                    }
                    std::cout << "Received keys: ";
                    for (const auto& key : received_keys) {
                        std::cout << key << " ";
                    }
                    std::cout << std::endl;
                    break;
                }
                case 'L': {
                    received_values.clear();
                    while (std::getline(ss, token, ',')) {
                        if (!token.empty()) {
                            received_values.push_back(token);
                        }
                    }
                    std::cout << "Received values: ";
                    for (const auto& value : received_values) {
                        std::cout << value << " ";
                    }
                    std::cout << std::endl;
                    // Ensure the number of keys and values match
                    if (received_keys.size() == received_values.size()) {
                        for (size_t i = 0; i < received_keys.size(); ++i) {
                            DataAprendisaje.insert({received_keys[i], received_values[i]});
                        }
                    }
                    std::cout << "Updated DataAprendisaje: ";
                    for (const auto& [key, value] : DataAprendisaje) {
                        std::cout << key << "=>" << value << " ";
                    }
                    std::cout << std::endl;
                    break;
                }
                default:
                    std::cout << "Received UDP message: " << message << std::endl;
                    break;
            }
        }
    }
}

void send_keep_alive(int tcp_socket) {
    while (true) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        
        std::stringstream ss;
        ss << "H:" << std::setw(2) << std::setfill('0') << now_tm->tm_hour << ":"
           << std::setw(2) << std::setfill('0') << now_tm->tm_min;
        
        std::string time_message = ss.str();
        send(tcp_socket, time_message.c_str(), time_message.size(), 0);
        std::cout << "Sent keep-alive message: " << time_message << std::endl; // Imprimir el mensaje enviado
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}

void print_client_config() {
    std::cout << "ClientConfig {" << std::endl;
    std::cout << "  nombres: [";
    for (size_t i = 0; i < client_config.nombres.size(); ++i) {
        std::cout << client_config.nombres[i];
        if (i < client_config.nombres.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]," << std::endl;

    std::cout << "  ips: [";
    for (size_t i = 0; i < client_config.ips.size(); ++i) {
        std::cout << client_config.ips[i];
        if (i < client_config.ips.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]," << std::endl;

    std::cout << "  elegido: [";
    for (size_t i = 0; i < client_config.elegido.size(); ++i) {
        std::cout << (client_config.elegido[i] ? "true" : "false");
        if (i < client_config.elegido.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
    std::cout << "}" << std::endl;
}