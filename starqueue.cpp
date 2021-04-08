#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VERSION "1.0.0"

using namespace std::chrono;

char ERROR_MSG[] = "ERROR.\r\n";
char OK_MSG[] = "OK.\r\n";
auto CHECKPOINT_TIME = duration_cast<milliseconds>(minutes{10}); 

std::string global_filename = "";
unsigned int global_port = 17827;
std::queue<std::string> global_queue;
std::mutex global_queue_mutex;
std::mutex checkpoint_file_mutex;

enum switch_types
{
    port,
    file,
    none,
    checkpoint,
};

void print_version()
{
    std::cout << "StarQueue, version " << VERSION << std::endl;
}

void show_help()
{
    print_version();
    std::cout << "Usage: starqueue [switches]" << std::endl;
    std::cout << " -h, --help          shows this help" << std::endl;
    std::cout << " -p, --port          sets the queue port" << std::endl;
    std::cout << " -f, --file          sets the queue checkpoint file" << std::endl;
    std::cout << " -c, --checkpoint    sets the checkpoint timeout in seconds" << std::endl;
    std::cout << " -a, --anyport       use any available port" << std::endl;
    std::cout << " -v, --version       prints version information" << std::endl;
}

void push_value(const std::string &value)
{
    // Complexity: O(1)
    global_queue_mutex.lock();
    global_queue.push(value);
    global_queue_mutex.unlock();
}

bool pop_value(std::string& out_value)
{
    bool is_empty = false;
    global_queue_mutex.lock();
    is_empty = global_queue.empty();
    if(!is_empty)
    {
        out_value = global_queue.front();
        global_queue.pop();
    }
    global_queue_mutex.unlock();
    return !is_empty;
}

std::string get_queue_size()
{
    global_queue_mutex.lock();
    size_t size = global_queue.size();
    global_queue_mutex.unlock();
    return std::to_string(size);
}

void clear_queue()
{
    global_queue_mutex.lock();
    std::queue<std::string> empty;
    std::swap(global_queue, empty );
    global_queue_mutex.unlock();
}

int test_save_file_permissions()
{
    FILE *fp = fopen(global_filename.c_str(), "a+");
    if (fp == NULL)
    {
        if (errno == EACCES)
        {
            std::cerr << "StarQueue: Cannot write to checkpoint file " << global_filename << ", permission denied." << std::endl;
        }
        else
        {
            std::cerr << "StarQueue: Cannot write to checkpoint file " << global_filename << ": " << strerror(errno) << "." << std::endl;
        }
        return 1;
    }
    return 0;
}

void save_datafile()
{
    //TODO compress checkpoint data
    std::lock(global_queue_mutex, checkpoint_file_mutex);
    std::cout << "StarQueue: Performing checkpoint to " << global_filename << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::queue<std::string> copied_queue = global_queue;
    global_queue_mutex.unlock();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "StarQueue: Queue released from checkpoint at " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms" << std::endl;
    std::ofstream queue_file;
    queue_file.open(global_filename);
    while(!copied_queue.empty())
    {
        queue_file << "ENQUEUE " << "\"" << copied_queue.front() << "\"\r\n";
        copied_queue.pop();
    }
    queue_file.flush();
    queue_file.close();
    end = std::chrono::steady_clock::now();
    std::cout << "StarQueue: Checkpoint performed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms" << std::endl;
    checkpoint_file_mutex.unlock();
}

void checkpoint_handler()
{
    while (true)
    {
        std::this_thread::sleep_for(CHECKPOINT_TIME);
        save_datafile();
    }
}

int execute_command(std::string &command, unsigned int socket_fd)
{
    std::vector<std::string> tokens;
    std::string current_token = "";
    bool escape_next_char = false;
    bool string_open = false;
    for (unsigned int i = 0; i < command.length(); ++i)
    {
        if (escape_next_char)
        {
            current_token += command[i];
            escape_next_char = false;
        }
        else if (command[i] == '\\')
        {
            current_token += command[i];
            escape_next_char = true;
        }
        else if (command[i] == '"')
        {
            string_open = !string_open;
            if (!string_open)
            {
                tokens.push_back(current_token);
                current_token.clear();
            }
            if (escape_next_char)
                current_token += command[i];
        }
        else if (command[i] == ' ' && !string_open)
        {
            if (!current_token.empty())
            {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
        else
        {
            current_token += command[i];
        }
    }
    if (!current_token.empty())
    {
        tokens.push_back(current_token);
        current_token.clear();
    }
    // Command format: COMMAND argument1 argument2 argument3 etc
    /*
    for (unsigned int i = 0; i < tokens.size(); ++i)
    {
        std::cout << "TOKEN " << i << "): " << tokens[i] << std::endl;
    }
    //*/
    if (tokens.empty())
        return 1;
    if (tokens[0] == "ENQUEUE" || tokens[0] == "enqueue")
    {
        if (tokens.size() < 2)
            return 1;
        else
            push_value(tokens[1]);
        return 0;
    }
    else if (tokens[0] == "DEQUEUE" || tokens[0] == "dequeue")
    {
        std::string value;
        if (pop_value(value))
        {
            send(socket_fd, "\"", 1, 0);
            send(socket_fd, value.c_str(), value.length(), 0);
            send(socket_fd, "\"\r\n", 3, 0);
            return 2;
        }
        else
        {
            return 1;
        }
    }
    else if (tokens[0] == "SIZE" || tokens[0] == "size")
    {
        std::string value = get_queue_size();
        send(socket_fd, value.c_str(), value.length(), 0);
            send(socket_fd, "\r\n", 2, 0);
        return 2;
    }
    else if (tokens[0] == "CLEAR" || tokens[0] == "clear")
    {
        clear_queue();
        return 0;
    }
    else if (tokens[0] == "PING" || tokens[0] == "ping")
    {
        return 0;
    }
    else if (tokens[0] == "CHECKPOINT" || tokens[0] == "checkpoint")
    {
        save_datafile();
        return 0;
    }
    return 1;
}

int parse_arguments(int argc, char **argv)
{
    std::string argument;
    switch_types requested_switch = none;
    for (int i = 1; i < argc; ++i)
    {
        argument = argv[i];
        if (requested_switch == none)
        {
            if (argument == "-h" || argument == "--help")
            {
                show_help();
                exit(0);
            }
            else if (argument == "-p" || argument == "--port")
            {
                requested_switch = port;
            }
            else if (argument == "-f" || argument == "--file")
            {
                requested_switch = file;
            }
            else if (argument == "-c" || argument == "--checkpoint")
            {
                requested_switch = checkpoint;
            }
            else if (argument == "-a" || argument == "--anyport")
            {
                global_port = 0;
            }
            else if (argument == "-v" || argument == "--version")
            {
                print_version();
                exit(0);
            }
            else
            {
                std::cerr << "StarQueue: Unknown switch: " << argument << std::endl;
            }
        }
        else
        {
            if (requested_switch == port)
            {
                try
                {
                    global_port = stoi(argument);
                }
                catch (const std::invalid_argument &ia)
                {
                    std::cerr << "StarQueue: Invalid port number: " << argument << std::endl;
                    return 1;
                }
            }
            else if (requested_switch == file)
            {
                global_filename = argument;
            }
            else if (requested_switch == checkpoint)
            {
                try
                {
                    CHECKPOINT_TIME = duration_cast<milliseconds>(seconds{stoi(argument)});
                }
                catch (const std::invalid_argument &ia)
                {
                    std::cerr << "StarQueue: Invalid checkpoint timeout value: " << argument << std::endl;
                    return 1;
                }
            }
            requested_switch = none;
        }
    }
    if (requested_switch != none)
    {
        std::cerr << "StarQueue: Expected argument for switch: " << argument << std::endl;
        return 1;
    }
    if (global_filename == "")
    {
        const char *homedir = getenv("HOME");
        if (homedir == NULL)
        {
            homedir = getpwuid(getuid())->pw_dir;
        }
        global_filename = (std::string)homedir + "/starqueue.qu";
    }
    return 0;
}

void handle_socket(int new_socket_fd)
{
    // TODO add alarm with timeout https://stackoverflow.com/questions/9163308/how-to-use-timeouts-with-read-on-a-socket-in-c-on-unix/9163476
    std::string str_buffer = "";
    char buffer[10];
    size_t bytes_read;
    bool escape_next_char = false;
    bool string_open = false;
    bool request_read_end = false;
    while ((bytes_read = recv(new_socket_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        for (unsigned int i = 0; i < bytes_read; ++i)
        {
            if (escape_next_char)
            {
                escape_next_char = false;
            }
            else if (buffer[i] == '\\')
            {
                escape_next_char = true;
            }
            else if (buffer[i] == '"')
            {
                string_open = !string_open;
            }
            else if (buffer[i] == '\n' || buffer[i] == '\r')
            {
                if (!string_open)
                {
                    request_read_end = true;
                    break;
                }
            }
            str_buffer += buffer[i];
        }
        memset(&buffer, 0, sizeof(buffer));
        if (request_read_end)
            break;
    }
    int retcode = execute_command(str_buffer, new_socket_fd);
    if (retcode == 1)
    {
        send(new_socket_fd, ERROR_MSG, strlen(ERROR_MSG), 0);
    }
    else if (retcode == 0)
    {
        send(new_socket_fd, OK_MSG, strlen(OK_MSG), 0);
    }
    shutdown(new_socket_fd, SHUT_RDWR);
    recv(new_socket_fd, buffer, sizeof(buffer), 0);
    close(new_socket_fd);
}

void start_socket()
{
    // Create socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "StarQueue: Socket creation failed" << std::endl;
        exit(1);
    }
    // Bind Socket
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(global_port);
    socklen_t len = sizeof(address);
    if (bind(sockfd, (struct sockaddr *)&address, len) < 0)
    {
        std::cerr << "StarQueue: Socket binding failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    getsockname(sockfd, (struct sockaddr *)&address, &len);
    global_port = ntohs(address.sin_port);
    // Inform the user
    std::cout << "StarQueue running on port " << global_port << ", backed to " << global_filename << std::endl;
    // Listen socket
    int new_socket_fd;
    if (listen(sockfd, 10) < 0)
    {
        std::cerr << "StarQueue: Read error: " << strerror(errno) << std::endl;
    }
    while (true)
    {
        if ((new_socket_fd = accept(sockfd, (struct sockaddr *)&address, &len)) < 0)
        {
            std::cerr << "StarQueue: Accept error" << std::endl;
        }
        std::thread child_thread(handle_socket, new_socket_fd);
        child_thread.detach();
    }
    close(sockfd);
}

void load_checkpoint()
{
    size_t loaded = 0;
    std::string line;
    std::ifstream checkpoint_file(global_filename);
    std::cout << "StarQueue: Attempting to load checkpoint file " << global_filename << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    while (std::getline(checkpoint_file, line))
    {
        // TODO fix multiline value loading
        if (execute_command(line, 0) == 0)
            ++loaded;
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "StarQueue: Loaded " << loaded << " values in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms" << std::endl;
}

int main(int argc, char **argv)
{
    int arg_out = parse_arguments(argc, argv);
    if (arg_out == 0)
    {
        print_version();
        if (test_save_file_permissions() == 0)
        {
            load_checkpoint();
            std::thread checkpoint_thread(checkpoint_handler);
            checkpoint_thread.detach();
            start_socket();
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return arg_out;
    }
    return 0;
}
