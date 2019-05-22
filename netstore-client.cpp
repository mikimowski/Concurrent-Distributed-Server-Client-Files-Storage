#include <iostream>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>



#include "err.h"
#include "helper.h"

constexpr uint16_t default_timeout = 5;
constexpr uint16_t max_timeout = 300;

using namespace std;
namespace po = boost::program_options;
namespace cp = communication_protocol;

struct ServerData {
    uint64_t available_space;
    const char* ip_addr;

    ServerData(const char* ip_addr, uint64_t available_space)
    : ip_addr(ip_addr),
    available_space(available_space)
    {}

    friend ostream& operator<< (ostream& out, ServerData &rhs) {
        out << "IP_ADDR = " << rhs.ip_addr << endl;
        out << "AVAILABLE_SPACE " << rhs.available_space << endl;

        return out;
    }

    friend bool operator>(const ServerData& lhs, const ServerData& rhs) {
        return lhs.available_space > rhs.available_space;
    }
};

struct ClientSettings {
    string mcast_addr;
    uint16_t cmd_port;
    string folder;
    uint16_t timeout;
};

ClientSettings parse_program_arguments(int argc, const char *argv[]) {
    ClientSettings client_settings{};

    po::options_description description {"Program options"};
    description.add_options()
        ("help,h", "Help screen")
        ("MCAST_ADDR,g", po::value<string>(&client_settings.mcast_addr)->required(), "Multicast address")
        ("CMD_PORT,p", po::value<uint16_t>(&client_settings.cmd_port)->required(), "UDP port on which servers are listening")
        ("OUT_FLDR,o", po::value<string>(&client_settings.folder)->required(), "Path to the directory in which files should be saved")
        ("TIMEOUT,t", po::value<uint16_t>(&client_settings.timeout)->default_value(default_timeout)->notifier([](uint16_t timeout) {
             if (timeout > max_timeout) {
                 cerr << "TIMEOUT out of range\n";
                 exit(1);
             }
        }),
         (("Maximum waiting time for information from servers\n"
           "  Min value: 0\n"
           "  Max value: " + to_string(max_timeout)) + "\n" +
           "  Default value: " + to_string(default_timeout)).c_str());

    po::variables_map var_map;
    try {
        po::store(po::parse_command_line(argc, argv, description), var_map);
        if (var_map.count("help")) {
            cout << description << endl;
            exit(0);
        }
        po::notify(var_map);
    } catch (po::required_option &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return client_settings;
}

void get_user_command() {
    string comm, param;

    cin >> comm;
    boost::algorithm::to_lower(comm);
    if (comm == "exit") {
        cout << comm;
    } else if (comm == "discover") {

    } else {
        cin >> param;
        boost::algorithm::to_lower(param);
        if (comm == "fetch") {
        } else if (comm == "upload") {
        } else if (comm == "remove") {
        } else {
            cout << "Unknown command\n";
        }
    }
}


class Client {
    const char* mcast_addr;
    const uint16_t cmd_port;
    const string folder;
    const uint16_t timeout;

    uint64_t next_cmd_seq;


    int mcast_sock_send;

    // For each filename stores last source_ip from which it was received
    unordered_map<string, const char*> last_search_results;

    void send_message_mcast_udp(const ComplexMessage &message) {
        int sock, optval;
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            syserr("socket");

        /* uaktywnienie rozgłaszania (ang. broadcast) */
        optval = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt broadcast");

        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
        optval = TTL;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt multicast ttl");

        // send on multicast
        /* ustawienie adresu i portu odbiorcy */
        struct sockaddr_in send_addr{};
        in_port_t send_port = cmd_port;

        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(send_port);
        if (inet_aton(mcast_addr, &send_addr.sin_addr) == 0)
            syserr("inet_aton");
        if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(message))
            syserr("sendto");
    }

    int send_message_mcast_udp(const SimpleMessage &message) {
        int sock, optval;

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            syserr("socket");

        /* uaktywnienie rozgłaszania (ang. broadcast) */
        optval = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt broadcast");

        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
        optval = TTL;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt multicast ttl");

        // send on multicast
        /* ustawienie adresu i portu odbiorcy */
        struct sockaddr_in send_addr{};
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(cmd_port);
        if (inet_aton(mcast_addr, &send_addr.sin_addr) == 0)
            syserr("inet_aton");
        if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(message))
            syserr("sendto");

        cerr << "mcast message sent" << endl;
        return sock;
    }

    void display_server_discovered_info(const char* server_ip, const char* server_mcast_addr, uint64_t server_space) {
        cout << "Found " << server_ip << " (" << server_mcast_addr << ") with free space " << server_space << endl;
    }

    bool correct_cmd_seq(uint64_t expected, uint64_t received) {
        return expected == received;
    }

    bool valid_discover_response(struct ComplexMessage& msg) {
    }


    void discover() {
//        int sock, optval;
//
//        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
//            syserr("socket");
//
//        /* uaktywnienie rozgłaszania (ang. broadcast) */
//        optval = 1;
//        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt broadcast");
//
//        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
//        optval = TTL;
//        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt multicast ttl");

        // send on multicast
        /* ustawienie adresu i portu odbiorcy */
        uint64_t tmp_message_seq;
        SimpleMessage message{tmp_message_seq, cp::discover_request};
            int sock = send_message_mcast_udp(message);
            // Co tu się dzieje, to sock ma przypisany jakis port więc chcemy na ten sam socket odbierac... albo chociaż na ten sam port?

//            if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
//                syserr("socket");
//            struct sockaddr_in local_address{};
//            local_address.sin_family = AF_INET;
//            local_address.sin_addr.s_addr = INADDR_ANY;
//            local_address.sin_port = htons(this->cmd_port);
//            if (bind(sock, (struct sockaddr*) &local_address, sizeof(local_address)) < 0)
//                syserr("bind");


//        struct sockaddr_in send_addr{};
//        send_addr.sin_family = AF_INET;
//        send_addr.sin_port = htons(cmd_port);
//        if (inet_aton(mcast_addr, &send_addr.sin_addr) == 0)
//            syserr("inet_aton");
//        if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(message))
//            syserr("sendto");
//
        // receive
        struct ComplexMessage msg_recv{};
        struct sockaddr_in src_addr{};
        socklen_t addrlen = sizeof(struct sockaddr_in);
        ssize_t recv_len;
        bool timeout_occ = false;

        {
            struct timeval timeval{};
            timeval.tv_usec = 1000;

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeval, sizeof(timeval)) < 0)
                syserr("setsockopt 'SO_RCVTIMEO'");
        }

        auto wait_start_time = std::chrono::high_resolution_clock::now();
        while (!timeout_occ) {
            auto curr_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed_time = curr_time - wait_start_time;
            if (elapsed_time.count() / 1000 >= timeout) {
                timeout_occ = true;
            } else {
                recv_len = recvfrom(sock, &msg_recv, sizeof(struct ComplexMessage), 0, (struct sockaddr*)&src_addr, &addrlen);
                if (recv_len == sizeof(msg_recv))
                    display_server_discovered_info(inet_ntoa(src_addr.sin_addr), msg_recv.data, msg_recv.param);
            }
        }
    }


//    void discover() {
//        // send on multicast
//        /* ustawienie adresu i portu odbiorcy */
//        uint64_t tmp_message_seq;
//        struct SimpleMessage message{tmp_message_seq, cp::discover_request};
//        send_message_mcast_udp(message);
//
//        // receive
//        int sock, optval;
//
//        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
//            syserr("socket");
//
//        /* uaktywnienie rozgłaszania (ang. broadcast) */
//        optval = 1;
//        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt broadcast");
//
//        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
//        optval = TTL;
//        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt multicast ttl");
//
//        struct ComplexMessage msg_recv{};
//        struct sockaddr_in src_addr{};
//        socklen_t addrlen = sizeof(struct sockaddr_in);
//        ssize_t recv_len;
//        bool timeout_occ = false;
//
//        {
//            struct timeval timeval{};
//            timeval.tv_usec = 1000;
//
//            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void*) &timeval, sizeof(timeval)) < 0)
//                syserr("setsockopt 'SO_RCVTIMEO'");
//        }
//
//        auto wait_start_time = std::chrono::high_resolution_clock::now();
//        while (!timeout_occ) {
//            auto curr_time = std::chrono::high_resolution_clock::now();
//            std::chrono::duration<double, std::milli> elapsed_time = curr_time - wait_start_time;
//            if (elapsed_time.count() / 1000 >= timeout) {
//                timeout_occ = true;
//            } else {
//                recv_len = recvfrom(sock, &msg_recv, sizeof(msg_recv), 0, (struct sockaddr*)&src_addr, &addrlen);
//                if (recv_len == sizeof(msg_recv))
//                    display_server_discovered_info(inet_ntoa(src_addr.sin_addr), msg_recv.data, msg_recv.param);
//            }
//        }
//    }


    set<ServerData, std::greater<>> silent_discover() {
        int sock, optval;

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            syserr("socket");

        /* uaktywnienie rozgłaszania (ang. broadcast) */
        optval = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt broadcast");

        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
        optval = TTL;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
            syserr("setsockopt multicast ttl");

        // send on multicast
        /* ustawienie adresu i portu odbiorcy */
        uint64_t tmp_message_seq;
        struct SimpleMessage msg_send{tmp_message_seq, cp::discover_request};
        struct sockaddr_in send_addr{};
        in_port_t send_port = cmd_port;

        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(send_port);
        if (inet_aton(mcast_addr, &send_addr.sin_addr) == 0)
            syserr("inet_aton");
        if (sendto(sock, &msg_send, sizeof(msg_send), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(msg_send))
            syserr("sendto");

        // receive
        struct ComplexMessage msg_recv{};
        struct sockaddr_in src_addr{};
        socklen_t addrlen = sizeof(struct sockaddr_in);
        ssize_t recv_len;
        bool timeout_occ = false;

        {
            struct timeval timeval{};
            timeval.tv_usec = 1000;

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeval, sizeof(timeval)) < 0)
                syserr("setsockopt 'SO_RCVTIMEO'");
        }

        set<ServerData, std::greater<>> servers;
        auto wait_start_time = std::chrono::high_resolution_clock::now();
        while (!timeout_occ) {
            auto curr_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed_time = curr_time - wait_start_time;
            if (elapsed_time.count() / 1000 >= timeout) {
                timeout_occ = true;
            } else {
                recv_len = recvfrom(sock, &msg_recv, sizeof(struct ComplexMessage), 0, (struct sockaddr*)&src_addr, &addrlen);
                if (recv_len == sizeof(msg_recv)) {
                    servers.insert(ServerData(inet_ntoa(src_addr.sin_addr), msg_recv.param)); // TODO jakaś fuinkcja parsująca i rzucająca wyjątek, try catch i handle it
                }
            }
        }

        return servers;
    }


    static void display_files_list(const char* recv_data, const char* source_ip) {
        int i = 0;
        while (recv_data[i] != '\0') {
            for (; recv_data[i] != '\n'; ++i)
                cout << recv_data[i];
            cout << " (" << (source_ip) << ")" << endl;
            ++i;
        }
    }

    // Memorizes only last occurrence of given filename
    void update_search_result(const string_view data, const char* source_ip) {
        int next_filename_start = 0, next_filename_end, len;
        string filename;

        while (data[next_filename_start] != '\0') {
            next_filename_end = data.find('\n', next_filename_start);
            len = next_filename_end - next_filename_start;
            filename = data.substr(next_filename_start, len);
            this->last_search_results[filename] = source_ip;
            next_filename_start = next_filename_end + 1;
        }
    }

    void display_list_of_known_files() {
        cout << "List of known files:" << endl;
        for (auto const& [filename, ip_source]: this->last_search_results)
            cout << filename << " from " << ip_source << endl;
    }

    void search(const string& pattern) {

//        /* uaktywnienie rozgłaszania (ang. broadcast) */
//        optval = 1;
//        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt broadcast");
//
//        /* ustawienie TTL dla datagramów rozsyłanych do grupy */
//        optval = TTL;
//        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
//            syserr("setsockopt multicast ttl");

        // send on multicast
        /* ustawienie adresu i portu odbiorcy */
        uint64_t tmp_message_seq;
        struct SimpleMessage message{tmp_message_seq, cp::files_list_request, pattern.c_str()};
        send_message_mcast_udp(message);
//        struct sockaddr_in send_addr{};
//        in_port_t send_port = (in_port_t)stoi(cmd_port);
//        if (pattern.length() > 0)
//            strcpy(message.data, pattern.c_str());
//
//        send_addr.sin_family = AF_INET;
//        send_addr.sin_port = htons(send_port);
//        if (inet_aton(mcast_addr, &send_addr.sin_addr) == 0)
//            syserr("inet_aton");
//        if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(message))
//            syserr("sendto");

        // receive
        int sock;
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            syserr("socket");

        struct SimpleMessage msg_recv{};
        struct sockaddr_in src_addr{};
        socklen_t addrlen = sizeof(struct sockaddr_in);
        ssize_t recv_len;
        bool timeout_occ = false;

        {
            struct timeval timeval{};
            timeval.tv_usec = 1000;

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeval, sizeof(timeval)) < 0)
                syserr("setsockopt 'SO_RCVTIMEO'");
        }


        auto wait_start_time = std::chrono::high_resolution_clock::now();
        while (!timeout_occ) {
            auto curr_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed_time = curr_time - wait_start_time;
            if (elapsed_time.count() / 1000 >= timeout) {
                timeout_occ = true;
            } else {
                recv_len = recvfrom(sock, &msg_recv, sizeof(struct ComplexMessage), 0, (struct sockaddr*)&src_addr, &addrlen);
                if (recv_len == sizeof(msg_recv)) {
                    const char* source_ip = inet_ntoa(src_addr.sin_addr);
                  //  display_files_list(msg_recv.data, source_ip);
                    update_search_result(msg_recv.data, source_ip);
                }
            }
        }
    }

    void fetch(const char* filename) {
        cout << "Starting fetch procedure" << endl;
        if (this->last_search_results.find(filename) == this->last_search_results.end()) {
            // TODO
            cout << "Uknown file handle" << endl;
        } else {
            // TODO
            /**
             * Workflow:
             * 1. UDP simple_cmd "GET" + nazwa pliku w data
             * 2. Odbierz od servera cmplx_cmd, cmt == CONNECTME, param = port tcp na którym server czeka, data = nazwa pliku który zostanie wysłany
             * 3. Nawiąrz połączenie TCP z serverem
             * 4. Pobieraj plik w dowhile i go zapisuje
             */

            int sock;
            const char* recv_ip = this->last_search_results[filename];

            if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                syserr("socket");

            // send
            uint64_t tmp_message_seq;
            struct SimpleMessage msg_send{tmp_message_seq, cp::file_get_request};
            struct sockaddr_in send_addr{};
            in_port_t send_port = cmd_port;
            strcpy(msg_send.data, filename);

            send_addr.sin_family = AF_INET;
            send_addr.sin_port = htons(send_port);
            if (inet_aton(recv_ip, &send_addr.sin_addr) == 0)
                syserr("inet_aton");
            if (sendto(sock, &msg_send, sizeof(msg_send), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(msg_send))
                syserr("sendto");
            cout << "File request sent" << endl;

            // receive
            struct ComplexMessage msg_recv{};
            struct sockaddr_in src_addr{};
            socklen_t addrlen = sizeof(struct sockaddr_in);
            ssize_t recv_len;

            recv_len = recvfrom(sock, &msg_recv, sizeof(msg_recv), 0, (struct sockaddr*)&src_addr, &addrlen);
            if (recv_len == sizeof(msg_recv)) {
                const char* source_ip = inet_ntoa(src_addr.sin_addr);
                cout << "Command: " << msg_recv.message << endl;
                cout << "Port: " << msg_recv.param << endl;
                cout << "Filename: " << msg_recv.data << endl;
            }
            cout << "TCP port received" << endl;
        }
        cout << "Ending fetch procedure" << endl;
    }

    static bool can_upload_file(ComplexMessage server_response) {
        return server_response.data == cp::file_add_acceptance;
    }

    /**
     * Sends UDP packet with file_add_request
     * @param server_ip
     * @param filename
     * @return
     */
    ComplexMessage ask_server_to_upload_file(const char* server_ip, const char* filename) {
        int sock;

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            syserr("socket");

        // send
        uint64_t tmp_message_seq;
        ComplexMessage msg_send{tmp_message_seq, cp::file_add_request};
        struct sockaddr_in send_addr{};
        in_port_t send_port = cmd_port;
        strcpy(msg_send.data, filename);

        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(send_port);
        if (inet_aton(server_ip, &send_addr.sin_addr) == 0)
            syserr("inet_aton");
        if (sendto(sock, &msg_send, sizeof(msg_send), 0, (struct sockaddr*) &send_addr, sizeof(send_addr)) != sizeof(msg_send))
            syserr("sendto");
        cout << msg_send.data << endl;
        cerr << "File upload request sent" << endl;

        // receive
        ComplexMessage msg_recv{};
        struct sockaddr_in src_addr{};
        socklen_t addrlen = sizeof(struct sockaddr_in);
        ssize_t recv_len;

        recv_len = recvfrom(sock, &msg_recv, sizeof(msg_recv), 0, (struct sockaddr*) &src_addr, &addrlen);
        if (recv_len == sizeof(msg_recv)) {
            cout << "Command: " << msg_recv.message << endl;
            cout << "Port: " << msg_recv.param << endl;
        }
        cerr << "File upload response received" << endl;

        return msg_recv;
    }

    void upload_file_via_tcp(const char* server_ip, in_port_t server_port, const char* filename) {
        cerr << "Starting uploading file via tcp..." << endl;

        cerr << "Ending uploading file via tcp..." << endl;
    }

    /**
     * Workflow:
     * 1. Discover servers and save them into the set
     * 2. If there are still not asked servers, ask next one with biggest available space whether file can be uploaded
     * 3. If no repeat this step
     * 4. If servers was found connect via TCP and send file
     * 5. Inform user whether file was successfully uploaded or not
     * @param file
     */
    void upload(const char* filename) {
        cout << "Starting upload procedure..." << endl;
        ComplexMessage server_response{};

        set<ServerData, std::greater<>> servers = silent_discover();
        for (const auto& server : servers) {
            server_response = ask_server_to_upload_file(server.ip_addr, filename);

            if (can_upload_file(server_response)) {
                upload_file_via_tcp(server.ip_addr, ntohl(server_response.param), filename);
                break;
            }
        }

        cout << "Ending upload procedure..." << endl;
    }

    void remove() {

    }

    void exit() {

    }

public:
    Client(const char* mcast_addr, uint16_t cmd_port, const string& folder, const uint16_t timeout)
            : next_cmd_seq(0),
              mcast_addr(mcast_addr),
              cmd_port(cmd_port),
              folder(folder),
              timeout(timeout)
    {}

    Client(ClientSettings& settings)
            : Client(settings.mcast_addr.c_str(), settings.cmd_port, settings.folder, settings.timeout)
    {}

    void run() {
//        cout << *this << endl;
//        this->last_search_results["file1_server1"] = "192.168.0.15";
//        fetch("file1_server1");
//
//        upload("client_file");
        discover();
    }


    void init() {

    }

    friend ostream& operator << (ostream &out, const Client &client) {
        out << "CLIENT INFO:" << endl;
        out << "MCAST_ADDR = " << client.mcast_addr << endl;
        out << "CMD_PORT = " << client.cmd_port << endl;
        out << "FOLDER = " << client.folder << endl;
        out << "TIMEOUT = " << client.timeout << endl;
        out << "NEXT_CMD_SEQ = " << client.next_cmd_seq << endl;

        return out;
    }
};


int main(int argc, const char *argv[]) {
    ClientSettings client_settings = parse_program_arguments(argc, argv);
   // get_user_command();
    Client client {client_settings};
    client.init();
    client.run();

    return 0;
}

