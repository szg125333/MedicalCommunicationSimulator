#pragma once
#include "IMessage.h"
#include <json/json.h>      // 确保 jsoncpp 路径正确
#include <memory>

class IMessageBuilder {
public:
    virtual ~IMessageBuilder() = default;

    // 注意：参数是 const Json::Value&，函数是 const
    virtual std::unique_ptr<IMessage> build(const Json::Value& json) const = 0;
};