#include <iostream>
#include "factory/MessageBuilderFactory.h"
#include "Worker/NewRequestWorker.h"
#include "Worker/NewNotifyWorker.h"
#include "messages/request/ImmutableFirstTypeRequest.h"  // 示例请求类型
#include "zmq.h"
#include <iostream>
#include <thread>
#include <chrono>


int main()
{
    // 1. 创建 ZeroMQ 上下文（整个程序一个即可）
    void* zmqContext = zmq_ctx_new();
    if (!zmqContext) {
        std::cerr << "Failed to create ZMQ context\n";
        return -1;
    }

    // 2. 创建并启动【通知监听器】——用于接收服务器推送的通知（如 ImagingStarted）
    NewNotifyWorker notifyWorker(zmqContext);
    notifyWorker.start();

    // 3. 创建并启动【请求发送器】——用于发送命令并等待响应
    NewRequestWorker requestWorker(zmqContext);
    requestWorker.start();

    // 4. 构造一个请求对象（例如：移动机架到 45 度）
    auto moveRequest = std::make_unique<ImmutableFirstTypeRequest>(45.0);

    // 5. 异步发送请求（非阻塞，内部会自动处理 send/recv）
    requestWorker.sendRequest(std::move(moveRequest));

    // 6. 程序运行一段时间，观察控制台输出
    std::cout << "\n>>> 系统运行中... 请观察控制台输出 <<<\n";
    std::cout << "可能看到：\n"
        << "  - [SEND REQUEST] ...\n"
        << "  - [RECV REPLY] ...\n"
        << "  - [NOTIFICATION] ...\n\n";

    // 运行 15 秒（期间可能收到响应和通知）
    std::this_thread::sleep_for(std::chrono::seconds(15));

    // 7. 优雅停止所有线程
    requestWorker.stop();
    notifyWorker.stop();

    // 8. 销毁 ZeroMQ 上下文
    zmq_ctx_destroy(zmqContext);


    std::cout << "Hello World!\n";

    std::string json = R"({
        "type": "rsp_MoveGantry",
        "requestId": "req-001",
        "success": true,
        "actualAngle": 45.0
    })";

    try {
        auto msg = MessageBuilderFactory::create(json);
        std::cout << "Success! Type: " << msg->getTypeName() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
