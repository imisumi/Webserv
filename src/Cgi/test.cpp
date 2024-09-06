// #include <iostream>
// #include <sys/epoll.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <cstring>
// #include <cstdlib>
// #include <chrono>
// #include <thread>

// #define MAX_EVENTS 10

// int main() {
//     int epoll_fd, stdin_fd;
//     struct epoll_event event, events[MAX_EVENTS];

//     // Create an epoll instance
//     epoll_fd = epoll_create1(0);
//     if (epoll_fd == -1) {
//         perror("epoll_create1");
//         return 1;
//     }

//     // Set stdin to non-blocking
//     stdin_fd = fileno(stdin);
//     fcntl(stdin_fd, F_SETFL, O_NONBLOCK);

//     // Add stdin file descriptor to epoll
//     event.events = EPOLLIN;
//     event.data.fd = stdin_fd;
//     if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stdin_fd, &event) == -1) {
//         perror("epoll_ctl");
//         return 1;
//     }

//     std::cout << "Waiting for input on stdin. Type something and press enter...\n";

//     int index = 0;
//     while (true) {
//         index++;

//         // Print the time
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//         std::cout << "Time: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;

//         int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
//         if (num_events == -1) {
//             perror("epoll_wait");
//             return 1;
//         }

//         for (int i = 0; i < num_events; ++i) {
//             if (events[i].data.fd == stdin_fd) {
//                 // Fork a child process to execute the script
//                 pid_t pid = fork();
//                 if (pid == -1) {
//                     perror("fork");
//                     return 1;
//                 }

//                 if (pid == 0) {
//                     // Child process
//                     const char *script = "./env.cgi";
//                     const char *script2 = "./sleep.py";
//                     const char *script_to_run = (index % 2 == 0) ? script : script2;

//                     char *const argv[] = { (char *)script_to_run, nullptr };
//                     char *const envp[] = { nullptr };

//                     // Execute the script
//                     execve(script_to_run, argv, envp);
//                     // If execve returns, an error occurred
//                     perror("execve");
//                     _exit(1); // Exit child process if execve fails
//                 }
//                 // Parent process continues
//                 else {
//                     // Non-blocking wait to clean up terminated child processes
//                     int status;
//                     while (waitpid(-1, &status, WNOHANG) > 0);
//                 }
//             }
//         }
//     }

//     close(epoll_fd);
//     return 0;
// }
