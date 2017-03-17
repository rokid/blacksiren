#include <thread>
#include <iostream>
#include <vector>
#include <fstream>

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>

#include "siren.h"
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

struct ChannelTest {
    ChannelTest(BlackSiren::SirenSocketReader& reader_) : reader(reader_) {}
    BlackSiren::SirenSocketReader& reader;
    std::thread th;
    void thread_handler();
    void channel_test();
};

void ChannelTest::thread_handler() {
    //BlackSiren::SirenSocketReader *reader = new BlackSiren::SirenSocketReader(&channel);
    //BlackSiren::SirenSocketReader reader(&channel);
    reader.prepareOnReadSideProcess();
    BlackSiren::Message *msg = nullptr;
    char *data = nullptr;
    for (;;) {
        if (reader.pollMessage(&msg) != BlackSiren::SIREN_CHANNEL_OK) {
            std::cout << "error!!!!!" << std::endl;
        } else {
            std::cout << "read message !!!" << std::endl;
            assert(msg != nullptr);

            std::cout << "read type " << msg->msg << " read len " << msg->len << std::endl;
            if (msg->len != 0) {
                std::cout << "read data " << data << std::endl;
            }
            msg->release();
        }
    }

}

void ChannelTest::channel_test() {
    std::thread t(&ChannelTest::thread_handler, this);
    th = std::move(t);
    th.join();
}

BlackSiren::SirenSocketChannel channel;
void test_channel() {
    if (!channel.open()) {
        BlackSiren::siren_printf(BlackSiren::SIREN_ERROR, "channel open failed");
        return;
    }

#if 1
    std::cout << "start test" << std::endl;
    int child  = fork();
#else
    std::thread t(&ChannelTest::thread_handler, this);
    th = std::move(t);
    th.join();
#endif
#if 1
    //in child
    if (child == 0) {
        BlackSiren::SirenSocketReader reader(&channel);
        ChannelTest test(reader);
        test.channel_test();
        //in parent
    } else if (child > 0) {
#endif
#if 0
        BlackSiren::SirenSocketWriter *writer = new BlackSiren::SirenSocketWriter(&channel);
        writer->prepareOnWriteSideProcess();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        int i = 0;
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            i++;
            BlackSiren::Message msg;
            msg.msg = i;
            char buff[] = "hello world";

            if (i % 2 == 0) {
                msg.len = 0;
                writer->writeMessage(&msg, nullptr);
            } else {
                msg.len = sizeof(buff);
                writer->writeMessage(&msg, buff);
            }
        }
#endif
        waitpid(child, nullptr, 0);
    }
}


std::ifstream test_recording_stream;
siren_t siren;
int init_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "init input stream");
    test_recording_stream.open("/data/debug0.pcm", std::ios::in | std::ios::binary);
    return 0;
}

void release_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "release input stream");
}

void start_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "start input stream");
}

void stop_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "stop input stream");
}

int read_input_stream(void *token,  char *buff, int len) {
    //siren_printf(BlackSiren::SIREN_INFO, "read input stream");
    test_recording_stream.read(buff, len);
    if (!test_recording_stream) {
        siren_printf(BlackSiren::SIREN_INFO, "read end of file");
        if (siren != 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            stop_siren_process_stream(siren);
            return 0;
        }
    }

    return 0;
}

void on_err_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "on err input stream");
}

void on_voice_event(void *token, int len, siren_event_t event,
                    void *buff, int has_sl, int has_voice, double sl_degree, int has_voice_print) {
    siren_printf(BlackSiren::SIREN_INFO, "on voice event");
}

void test_init() {
    siren_input_if_t input_callback;
    input_callback.init_input = init_input_stream;
    input_callback.release_input = release_input_stream;
    input_callback.start_input = start_input_stream;
    input_callback.stop_input = stop_input_stream;
    input_callback.on_err_input = on_err_input_stream;
    input_callback.read_input = read_input_stream;

    siren = init_siren(nullptr, "/data/test.json", &input_callback);
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_INFO, "init siren failed");
        return;
    }

    destroy_siren(siren);
}


void test_recording() {
    siren_input_if_t input_callback;
    siren_proc_callback_t proc_callback;

    input_callback.init_input = init_input_stream;
    input_callback.release_input = release_input_stream;
    input_callback.start_input = start_input_stream;
    input_callback.stop_input = stop_input_stream;
    input_callback.on_err_input = on_err_input_stream;
    input_callback.read_input = read_input_stream;

    proc_callback.voice_event_callback = on_voice_event;
    siren = init_siren(nullptr, "/data/test.json", &input_callback);
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_INFO, "init siren failed");
        return;
    }

    siren_printf(BlackSiren::SIREN_INFO, "start recording test");
    start_siren_process_stream(siren, &proc_callback);

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

int main(void) {
    //test_common();
    //test_channel();
    //test_thread_start();
    //test_thread_hardware_concurrency();
    //test_fork_socketpair();

    test_init();
    //test_recording();
}
