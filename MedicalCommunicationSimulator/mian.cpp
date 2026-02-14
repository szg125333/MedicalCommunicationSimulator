// main.cpp (PacsServer 入口)
#include <zmq.h>
#include "factory/MessageBuilderFactory.h"
#include "messages/request/QuerySeriesRequest.h"
#include "messages/FileMeta.h"
#include <json/json.h>
#include <iostream>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <fstream>

// 全局运行标志
std::atomic<bool> running(true);

void signalHandler(int) {
    running = false;
}

bool isValidStoragePath(const std::string& path, const std::string& base) {
    try {
        auto canonicalBase = std::filesystem::canonical(base);
        auto canonicalPath = std::filesystem::canonical(path);
        return std::filesystem::equivalent(canonicalBase, canonicalPath) ||
            (canonicalPath.string().rfind(canonicalBase.string(), 0) == 0);
    }
    catch (...) {
        return false; // 如果路径不存在或解析失败，直接判定为非法
    }
}


void sendErrorResponse(void* socket, const std::string& msg) {
    Json::Value error;
    error["error"] = msg;
    Json::StreamWriterBuilder builder;
    std::string jsonStr = Json::writeString(builder, error);
    zmq_send(socket, jsonStr.c_str(), jsonStr.size(), 0);
}

void handleQuerySeries(void* socket, const QuerySeriesRequest* req) {
    std::string basePath = "/root/PacsServer/Storage"; // 改成绝对路径
    std::string seriesPath = basePath + "/" +
        req->getPatientId() + "/" +
        req->getStudyUid() + "/" +
        (req->getSeriesUid().empty() ? "" : req->getSeriesUid());

    // 使用封装好的函数
    if (!isValidStoragePath(seriesPath, basePath)) {
        sendErrorResponse(socket, "Invalid path");
        return;
    }

    if (!std::filesystem::exists(seriesPath)) {
        sendErrorResponse(socket, "Series not found");
        return;
    }

    int fileIndex = 0;
    for (const auto& entry : std::filesystem::directory_iterator(seriesPath)) {
        if (entry.path().extension() == ".dcm") {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) {
                sendErrorResponse(socket, "Failed to open file");
                continue;
            }

            // 使用 FileMeta 类
			FileMeta meta(entry.path().filename().string(), fileIndex++, 100);  //100这里是个示例值，实际可以根据需要设置
            std::string metaStr = meta.toJson();
            zmq_send(socket, metaStr.c_str(), metaStr.size(), ZMQ_SNDMORE);


            // 再发二进制
            std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            zmq_send(socket, buffer.data(), buffer.size(), 0);
        }
    }
}


int main() {
    // 注册 Ctrl+C 信号处理
    std::signal(SIGINT, signalHandler);

    void* ctx = zmq_ctx_new();
    void* socket = zmq_socket(ctx, ZMQ_REP);
    zmq_bind(socket, "tcp://*:5557");
    std::cout << "PACS Server listening on port 5557...\n";

    zmq_pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 } };

    while (running) {
        int rc = zmq_poll(items, 1, 1000); // 1秒超时
        if (rc > 0 && (items[0].revents & ZMQ_POLLIN)) {
            zmq_msg_t msg;
            zmq_msg_init(&msg);
            if (zmq_msg_recv(&msg, socket, 0) == -1) {
                zmq_msg_close(&msg);
                continue;
            }

            std::string jsonStr(static_cast<char*>(zmq_msg_data(&msg)), zmq_msg_size(&msg));
            zmq_msg_close(&msg);

            try {
                auto request = MessageBuilderFactory::create(jsonStr);
                if (request->getTypeName() == QuerySeriesRequest::TYPE_NAME) {
                    handleQuerySeries(socket, dynamic_cast<QuerySeriesRequest*>(request.get()));
                }
                else {
                    sendErrorResponse(socket, "Unsupported message type");
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                sendErrorResponse(socket, e.what());
            }
        }
    }

    std::cout << "Shutting down PACS Server...\n";
    zmq_close(socket);
    zmq_ctx_destroy(ctx);
    return 0;
}
