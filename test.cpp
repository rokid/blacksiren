#include <thread>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>

#include "sutils.h"
#include "lfqueue.h"
#include "easyr2_queue.h"
#include "siren_channel.h"

void test_fork_socketpair() {
    int sockets[2], child;
    char buf[1024] = {0};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        perror("opening socket pair failed");
        exit(1);
    }

    if ((child = fork()) == -1) {
        perror("fork failed");
        //in parent
    } else if (child) {
        std::cout << "child = " << child << std::endl;
        int status;
        close(sockets[1]);
        if (read(sockets[0], buf, 1024) < 0) {
            perror("read stream message failed");
        }

        std::cout << "parent read " << buf << std::endl;
        if (write(sockets[0], "hello from parent", 18) < 0) {
            perror("write stream message failed");
        }

        std::cout << "in wait" << std::endl;
        wait(&status);
        close(sockets[0]);
        //in child
    } else {
        close(sockets[0]);
        if (write(sockets[1], "hello from child", 17) < 0) {
            perror("write stream message failed");
        }

        if (read(sockets[1], buf, 1024) < 0) {
            perror("read stream message failed");
        }

        std::cout << "child read " << buf << std::endl;
        close(sockets[1]);
        std::cout << "child end.." << std::endl;
    }
}

void test_thread_hardware_concurrency() {
    std::cout << "thread hwc = " << std::thread::hardware_concurrency() << std::endl;
}

void __test_th_fn1() {
    std::cout << "in __test_th_fn1" << std::endl;
}

void test_thread_start() {
    std::cout << "before create t1" << std::endl;
    std::thread t1(__test_th_fn1);
    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    std::cout << "after create r1" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    t1.join();
}

void test_fn_producer(BlackSiren::LFQueue &queue) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (int i = 0; i < 100; i++) {
        std::cout << "push " << i << std::endl;
        int *new_i = new int (i);
        queue.push((void*)new_i);
    }
    std::cout << "producer end" << std::endl;
}

void test_fn_consumer(BlackSiren::LFQueue &queue) {
    for (int i = 0; i < 100; i++) {
        int *new_i = nullptr;
        std::cout << "START READ" << std::endl;
        int status = 0;
        status = queue.pop((void **)&new_i, nullptr);
        if (new_i == nullptr) {
            std::cout << "nullptr:" << status << std::endl;
            continue;
        }
        std::cout << "READ " << *new_i << std::endl;
        delete new_i;
    }
}

void test_common() {
    BlackSiren::siren_printf(BlackSiren::SIREN_INFO, "hello world");
    BlackSiren::LFQueue queue(1024, nullptr);
    //BlackSiren::easyr2_queue queue2;

    std::thread t1(test_fn_producer, std::ref(queue));
    std::thread t2(test_fn_consumer, std::ref(queue));

    t1.join();
    t2.join();
}

void test_channel() {
    BlackSiren::SirenSocketChannel channel;
    if (!channel.open()) {
        BlackSiren::siren_printf(BlackSiren::SIREN_ERROR, "channel open failed");
        return;
    }

    BlackSiren::SirenSocketReader *reader = new BlackSiren::SirenSocketReader(&channel);
    BlackSiren::SirenSocketWriter *writer = new BlackSiren::SirenSocketWriter(&channel);

    if (reader == nullptr || writer == nullptr) {
        BlackSiren::siren_printf(BlackSiren::SIREN_ERROR, "either writer or reader is null or both null");
        return;
    }


    std::cout << "start test" << std::endl;
    int child  = fork();
    //in child
    if (child == 0) {
        reader->prepareOnReadSideProcess();
        BlackSiren::Message *msg = nullptr;
        char *data = nullptr;
        for (;;) {
            if (reader->pollMessage(&msg, &data) != BlackSiren::SIREN_CHANNEL_OK) {
                std::cout << "error!!!!!" << std::endl;
            } else {
                std::cout << "read message !!!" << std::endl;
                assert(msg != nullptr);

                std::cout << "read type " << msg->msg << " read len " << msg->len << std::endl;
                if (msg->len != 0) {
                    std::cout << "read data "<< data<<std::endl;
                } 
                msg->release();
            }
        }
        //in parent
    } else if (child > 0) {
        writer->prepareOnWriteSideProcess();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        int i = 0;
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            i++;
            BlackSiren::Message msg;
            msg.msg = i;
            char buff[] = "hello world";

            if (i%2==0) {
                msg.len = 0;
                writer->writeMessage(&msg, nullptr);
            } else {
                msg.len = sizeof(buff); 
                writer->writeMessage(&msg, buff);
            }
        }

        waitpid(child, nullptr, 0);
    }
}

namespace MemTrace {

class MemTraceObject {
public:
    MemTraceObject() {
    }
    static void operator delete(void *ptr);
    static void *operator new(std::size_t count);
private:
    static int alloc_ref;
public:
    static int get_alloc_ref() {
        return alloc_ref;
    }
};

int MemTraceObject::alloc_ref = 0;
void *MemTraceObject::operator new(std::size_t count) {
    alloc_ref ++;
    return ::operator new(count);
}

void MemTraceObject::operator delete(void *ptr) {
    alloc_ref --;
    ::operator delete(ptr);
}

class Test : public MemTraceObject {
public:
    Test(int _a) : a(_a) {}
private:
    int a = 0;
};

void test_cpp() {
    Test *t = new Test(10);
    Test *t2 = new Test(12);

    delete t;
    std::cout << "---->" << MemTraceObject::get_alloc_ref() << std::endl;
}

}
int main(void) {
    //test_common();
    test_channel();
    //test_thread_start();
    //test_thread_hardware_concurrency();
    //test_fork_socketpair();
}
