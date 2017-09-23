#include "WorkerManager.h"
#include "../3rd/rapidjson/document.h"
#include <muduo/base/Logging.h>

WorkerManager::WorkerManager(muduo::net::EventLoop *loop,
                             const muduo::net::InetAddress &listenAddr)
    : m_httpDispatch(kTimeoutSecond, loop, listenAddr, "WorkerManager")
{
    m_httpDispatch.AddHander("/start",
                             std::tr1::bind(&WorkerManager::HandleStartPush,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/end",
                             std::tr1::bind(&WorkerManager::HandleEndPush,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/heartbeat",
                             std::tr1::bind(&WorkerManager::HandleHeartbeat,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/get_ip_to_publish",
                             std::tr1::bind(&WorkerManager::HandleGetIpToPublish,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/get_stream_to_play",
                             std::tr1::bind(&WorkerManager::HandleGetStreamToPlay,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));
    m_httpDispatch.Start();
}

void WorkerManager::HandleStartPush(const boost::shared_ptr<HttpRequester> &req,
                                    boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleStartPush";
    PutDataToGetWorkerInfo(req);
    m_httpDispatch.ResponseOk(resp);
}

void WorkerManager::HandleEndPush(const boost::shared_ptr<HttpRequester> &req,
                                  boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleEndPush";
    PutDataToGetWorkerInfo(req);
    m_httpDispatch.ResponseOk(resp);
}

void WorkerManager::HandleHeartbeat(const boost::shared_ptr<HttpRequester> &req,
                                    boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleHeartbeat";
    PutDataToGetWorkerInfo(req);
    m_httpDispatch.ResponseOk(resp);
}

void WorkerManager::HandleGetIpToPublish(const boost::shared_ptr<HttpRequester>& req,
                                         boost::shared_ptr<HttpResponser>& resp)
{
    LOG_INFO << "HandleGetIpToPublish";
    std::string ip;
    if (m_getWorkerInfo.GetPublishIp(ip))
    {
        resp->SetStatusCode(HttpResponser::StatusCode_200Ok);
        std::string body = "{\"code\":0, \"ip\":\"" + ip + "\"}";
        resp->SetBody(body.c_str());
    }
    else
    {
        std::string body = "{\"code\":1, \"message\":\"No stream server\"}";
        resp->SetBody(body.c_str());
    }
}

void WorkerManager::HandleGetStreamToPlay(const boost::shared_ptr<HttpRequester>& req,
                                          boost::shared_ptr<HttpResponser>& resp)
{
    LOG_INFO << "HandleGetStreamToPlay";
    WorkerInfo::StreamInfo streamInfo;
    rapidjson::Document d;
    d.Parse(req->GetBody().c_str());
    bool valid = !d.HasParseError();
    if (valid)
    {
        valid = d.HasMember("stream_id") && d["stream_id"].IsString() &&
            d.HasMember("app_id") && d["app_id"].IsString();

        if (valid)
        {
            valid = m_getWorkerInfo.GetSteamToPlay(
                d["app_id"].GetString(), d["stream_id"].GetString(), streamInfo);
        }
        else
        {
            std::string body = "{\"code\":1, \"message\":\"json parse error\"}";
            resp->SetBody(body.c_str());
            return;
        }
    }
    else
    {
        std::string body = "{\"code\":1, \"message\":\"json parse error\"}";
        resp->SetBody(body.c_str());
        return;
    }

    if (valid)
    {
        std::string body = "{\"code\":0, \"ip\":\"" + streamInfo.ip + "\"}";
        resp->SetBody(body.c_str());
    }
    else
    {
        std::string body = "{\"code\":1, \"message\":\"No stream\"}";
        resp->SetBody(body.c_str());
    }
}

void WorkerManager::PutDataToGetWorkerInfo(
    const boost::shared_ptr<HttpRequester> &req)
{
    GetWorkerInfo::QueueMessagePtr message(new GetWorkerInfo::QueueMessage);
    message->type = GetWorkerInfo::MessageType_Data;
    message->message.ip = req->GetPeerIp();
    message->message.port = 1985;
    m_getWorkerInfo.Put(message);
}
