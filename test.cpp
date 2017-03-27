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
#include "siren_channel.h"

#include "mic/mic_array.h"

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

using BlackSiren::LFQueue;
struct LFQueueTest {
    LFQueueTest() : queue(1024 * 1024, nullptr) {
        //    easyr2_queue_init(&queue, 1024 * 1024, nullptr);
    }
    LFQueue queue;
    //easyr2_queue queue;
    void test_fn_producer();
    void test_fn_consumer();
    void launchProducer();

    std::thread producer;
};

void LFQueueTest::test_fn_producer() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (int i = 0; i < 1000 * 1024; i++) {
        //std::cout << "push " << i << std::endl;
        int *new_i = new int (i);
        queue.push((void *)new_i);
        //easyr2_queue_push(&queue, (void *)new_i);
    }
    std::cout << "producer end" << std::endl;
}

void LFQueueTest::test_fn_consumer() {
    for (int i = 0; i < 1000 * 1024; i++) {
        int *new_i = nullptr;
        int status = 0;
        status = queue.pop((void **)&new_i, nullptr);
        //status = easyr2_queue_pop(&queue, (void **)&new_i, nullptr);
        if (new_i == nullptr) {
            std::cout << "nullptr:" << status << std::endl;
            break;
        }
        std::cout << "READ " << *new_i << std::endl;
        delete new_i;
    }
}

void LFQueueTest::launchProducer() {
    std::thread t1(&LFQueueTest::test_fn_producer, this);
    producer = std::move(t1);
}

void test_common() {
    BlackSiren::siren_printf(BlackSiren::SIREN_INFO, "hello world");
    LFQueueTest test;
    test.launchProducer();
    test.test_fn_consumer();
    test.producer.join();
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
    std::ofstream consumer_dump;
    int next = 10;
    bool dump = false;
    consumer_dump.open("/data/consumer.dump", std::ios::out);
    reader.prepareOnReadSideProcess();
    BlackSiren::Message *msg = nullptr;
    char *data = nullptr;
    for (;;) {
        if (next <= 0) {
            std::cout << "end!!!" << std::endl;
            break;
            //break;
        }
        if (reader.pollMessage(&msg) != BlackSiren::SIREN_CHANNEL_OK) {
            std::cout << "error!!!!!" << std::endl;
            next--;
            //std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
            //break;

        } else {
            std::cout << "read message !!!" << std::endl;
            assert(msg != nullptr);

            std::cout << "read type " << msg->msg << " read len " << msg->len << std::endl;
            if (msg->len != 0) {
                //std::cout << "read data " << msg->data << std::endl;
                //            consumer_dump.write(msg->data, msg->len);
                //            consumer_dump << std::endl;
            }
            delete (char *)msg;
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
#if 1
        BlackSiren::SirenSocketWriter *writer = new BlackSiren::SirenSocketWriter(&channel);
        writer->prepareOnWriteSideProcess();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        int i = 0;
        for (;;) {
            i = 0;
            for (int k = 0; k < 1024 * 100; k++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                i++;
                BlackSiren::Message *msg;
                msg = BlackSiren::allocateMessage(1, i + 1);
                std::string t("a");
                std::string r;
                for (int j = 0; j < i; j++) {
                    r += t;
                }

                memcpy(msg->data, r.c_str(), i + 1);
                writer->writeMessage(msg);
                delete (char *)msg;
            }
        }
#endif
        waitpid(child, nullptr, 0);
    }
}

int magic = 233;
std::ofstream recordingDebugStream;
std::ifstream test_recording_stream;
siren_t siren;
int init_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "init input stream");
    test_recording_stream.open("/data/debug0.pcm", std::ios::in | std::ios::binary);
    recordingDebugStream.open("/data/test.pcm", std::ios::out | std::ios::binary);
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

bool stopRecording = false;
int read_input_stream(void *token,  char *buff, int len) {
    //siren_printf(BlackSiren::SIREN_INFO, "read input stream");
    test_recording_stream.read(buff, len);
    if (!test_recording_stream) {
        siren_printf(BlackSiren::SIREN_INFO, "read end of file");
        if (siren != 0) {
            std::this_thread::sleep_for(std::chrono::seconds(100));
            stop_siren_process_stream(siren);
            return 0;
        }
    }

    return 0;
}

void on_err_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "on err input stream");
}

const char *eventToString(siren_event_t event) {
    switch(event) {
    case SIREN_EVENT_VAD_START:
        return "vad_start";
    case SIREN_EVENT_VAD_DATA:
        return "vad_data";
    case SIREN_EVENT_VAD_END:
        return "vad_end";
    case SIREN_EVENT_VAD_CANCEL:
        return "vad_cancel";
    case SIREN_EVENT_WAKE_VAD_START:
        return "wake_vad_start";
    case SIREN_EVENT_WAKE_VAD_DATA:
        return "wake_vad_data";
    case SIREN_EVENT_WAKE_VAD_END:
        return "wake_vad_end";
    case SIREN_EVENT_WAKE_PRE:
        return "wake_pre";
    case SIREN_EVENT_WAKE_NOCMD:
        return "wake_nocmd";
    case SIREN_EVENT_WAKE_CMD:
        return "wake_cmd";
    case SIREN_EVENT_SLEEP:
        return "sleep";
    case SIREN_EVENT_HOTWORD:
        return "hotword";
    case SIREN_EVENT_SR:
        return "sr";
    case SIREN_EVENT_VOICE_PRINT:
        return "voice_print";
    case SIREN_EVENT_DIRTY:
        return "dirty";
    default: {
        siren_printf(BlackSiren::SIREN_WARNING, "unknown event %d", (int)event);
        return "unknown event";
    }
    }
}

void on_voice_event(void *token, int len, siren_event_t event,
                    void *buff, int has_sl, int has_voice,
                    double sl_degree, double energy, double threshold, int has_voice_print) {
    siren_printf(BlackSiren::SIREN_INFO,
                 "rcv event %s with len %d, has_sl %d, has_voice %d, energy %f, threshold %f",
                 eventToString(event), len, has_sl, has_voice, energy, threshold);
    if (has_sl == 1) {
        siren_printf(BlackSiren::SIREN_INFO, "sl degree %f", sl_degree);
    }

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
    siren = init_siren(nullptr, nullptr, &input_callback);
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

mic_array_module_t *module;
bool mic_open = false;
mic_array_device_t *mic_array_device;

static inline int mic_array_device_open (const hw_module_t *module, struct mic_array_device_t **device) {
    return module->methods->open (module, MIC_ARRAY_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}

int init_xmos_input_stream(void *token) {
    if (hw_get_module (MIC_ARRAY_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) == 0) {
        siren_printf (BlackSiren::SIREN_INFO, "find mic_array");
        //open mic array
        if (0 != mic_array_device_open(&module->common, &mic_array_device)) {
            siren_printf(BlackSiren::SIREN_ERROR, "open mic array failed");
            return -1;
        } else {
            mic_open = true;
        }
    } else {
        return -1;
    }

    //recordingDebugStream.open("/data/test.pcm", std::ios::out | std::ios::binary);
    return 0;
}

void release_xmos_input_stream(void *token) {
}

void start_xmos_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "start input stream");
    mic_array_device->start_stream(mic_array_device);
    siren_printf(BlackSiren::SIREN_INFO, "end of start input stream");
}

void stop_xmos_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "stop input stream");
    mic_array_device->stop_stream(mic_array_device);
}

int read_xmos_input_stream(void *token,  char *buff, int len) {
    //mic_array_device->read_stream(mic_array_device, buff, (unsigned long long *)&len);
    //memset(buff, 0, len);
    unsigned long long size = 0;
    //buff[len/2] = 'a';
    //buff[len/2 + 1] = 'b';
    //buff[len/2 + 2] = 'c';
    //buff[len/2 + 3] = 'd';
    //siren_printf(BlackSiren::SIREN_INFO, "before read stream");
    mic_array_device->read_stream(mic_array_device, buff, size);
    //recordingDebugStream.write(buff, len);
    //buff[len/2 + 4] = '\0';
    //std::cout<<buff + len/2<<std::endl;
    return 0;
}

void on_err_xmos_input_stream(void *token) {
    siren_printf(BlackSiren::SIREN_INFO, "on err input stream");
}

void on_siren_state_changed_fn(void *token, int current) {
    int *pm = (int *)token;
    siren_printf(BlackSiren::SIREN_INFO, "magic is %d", *pm);
    siren_printf(BlackSiren::SIREN_INFO, "set state to %d", current);
}

void test_xmos() {

    siren_input_if_t input_callback;
    siren_proc_callback_t proc_callback;
    siren_state_changed_callback_t state_changed_callback;

    state_changed_callback.state_changed_callback = on_siren_state_changed_fn;
    input_callback.init_input = init_xmos_input_stream;
    input_callback.release_input = release_xmos_input_stream;
    input_callback.start_input = start_xmos_input_stream;
    input_callback.stop_input = stop_xmos_input_stream;
    input_callback.on_err_input = on_err_xmos_input_stream;
    input_callback.read_input = read_xmos_input_stream;

    proc_callback.voice_event_callback = on_voice_event;
    siren = init_siren((void *)&magic, nullptr, &input_callback);
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_INFO, "init siren failed");
        return;
    }
    siren_printf(BlackSiren::SIREN_INFO, "start recording test");
    start_siren_process_stream(siren, &proc_callback);

#if 0
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        siren_printf(BlackSiren::SIREN_INFO, "test set state awake sync");
        set_siren_state(siren, SIREN_STATE_AWAKE, nullptr);
        siren_printf(BlackSiren::SIREN_INFO, "now state is in wake");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        siren_printf(BlackSiren::SIREN_INFO, "test set state sleep async");
        set_siren_state(siren, SIREN_STATE_SLEEP, &state_changed_callback);
        std::this_thread::sleep_for(std::chrono::seconds(100));
    });

    t.join();
#endif
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

}

void test_mic() {
    if (hw_get_module (MIC_ARRAY_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) == 0) {
        siren_printf (BlackSiren::SIREN_INFO, "find mic_array");
        //open mic array
        if (0 != mic_array_device_open(&module->common, &mic_array_device)) {
            siren_printf(BlackSiren::SIREN_ERROR, "open mic array failed");
            return;
        } else {
            mic_open = true;
        }
    } else {
        return;
    }

    recordingDebugStream.open("/data/test.pcm", std::ios::out | std::ios::binary);
    if (0 != mic_array_device->start_stream(mic_array_device)) {
        std::cout<<"mic_array start stream failed"<<std::endl;
    }
    int frameSize = 8 * 4 * 480;//mic_array_device->get_stream_buff_size(mic_array_device);
    siren_printf(BlackSiren::SIREN_INFO, "use frame cnt %d", frameSize);
    char *buff = new char[frameSize];
    for (;;){
        int size = 0;
        mic_array_device->read_stream(mic_array_device, buff, frameSize);
        recordingDebugStream.write(buff, frameSize);     
    }
}

int main(void) {
    //test_common();
    //test_channel();
    //test_thread_start();
    //test_thread_hardware_concurrency();
    //test_fork_socketpair();

    //test_init();
    //test_recording();
    test_xmos();
    //test_mic();
}
