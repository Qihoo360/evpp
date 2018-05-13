#include "service.h"
#include "evpp/libevent.h"
namespace evpp {
namespace evpphttp {
Service::Service(const std::string& listen_addr, const std::string& name, uint32_t thread_num):listen_addr_(listen_addr), name_(name), thread_num_(thread_num) {
}

bool Service::Start(const ConnectionCallback& cb) {
    listen_loop_ = new EventLoop();
    assert(listen_loop_ != nullptr);
    listen_thr_ = new std::thread([listen_loop = listen_loop_]() {
        listen_loop->Run();
    });
    assert(listen_thr_ != nullptr);
    tcp_srv_ = new TCPServer(listen_loop_, listen_addr_/*ip:port*/, name_, thread_num_);
    assert(tcp_srv_ != nullptr);
    tcp_srv_->SetConnectionCallback(cb);
    tcp_srv_->SetMessageCallback(std::bind(&Service::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    if (!tcp_srv_->Init()) {
        LOG_WARN << "tcpserver on " << listen_addr_ << " init failed";
        return false;
    }
    if (!tcp_srv_->Start()) {
        LOG_WARN << "tcpserver on " << listen_addr_ << " start failed";
        return false;
    }
    LOG_INFO << "http server start on " << listen_addr_ << " suc";
    return true;
}

Service::~Service() {
    if (!is_stopped_) {
        Stop();
    }
}

void Service::Stop() {
    DLOG_TRACE << "http service is stopping";
    tcp_srv_->Stop();
    listen_loop_->Stop();
    listen_thr_->join();
    delete listen_thr_;
    delete listen_loop_;
    delete tcp_srv_;
    callbacks_.clear();
    DLOG_TRACE << "http service stopped";
    is_stopped_ = true;
}

void Service::RegisterHandler(const std::string& uri, const HTTPRequestCallback& callback) {
    callbacks_[uri] = callback;
}

int Service::RequestHandler(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, HttpRequest& hr) {
    std::map<std::string, std::string> empty_field_value;
    if (hr.Parse(buf) != 0) {
        HttpResponse resp(hr);
        resp.SendReply(conn, 400/*bad request*/, empty_field_value, "");
        return -1;
    }
    if (hr.completed()) {
        auto path = std::move(hr.url_path());
        auto cb = callbacks_.find(path);
        HttpResponse resp(hr);
        if (cb == callbacks_.end()) {
            resp.SendReply(conn, 404/*NOT FOUND*/, empty_field_value, "");
        } else {
            cb->second(conn->loop(), hr,
                       [conn, resp](const int response_code, const std::map<std::string, std::string>& response_field_value, const std::string& response_data) mutable
            {
                resp.SendReply(conn, response_code, response_field_value, response_data);
            });
        }
        return 0;
    }
    //continue
    auto expect = hr.field_value.find("Expect");
    if (expect != hr.field_value.end() && !hr.is_send_continue() && evutil_ascii_strcasecmp(expect->first.data(), "100-continue")) {
        HttpResponse resp(hr);
        resp.SendReply(conn, 100/*CONTINUE*/, empty_field_value, "");
		hr.set_continue();
    }
    return 1; //need recv more data
}


void Service::OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf) {
    int ret = 0;
    //LOG_TRACE << "recv message:" << buf->ToString();
    if (!conn->context().IsEmpty()) {
        auto context = conn->context();
        HttpRequest *hr = context.Get<HttpRequest *>();
        ret = RequestHandler(conn, buf, *hr);
        if (ret != 0) {
            return;
        }
        delete hr;
        Any empty;
        conn->set_context(empty);
    }
    while (buf->size() > 0) {
        HttpRequest hr;
        hr.set_remote_ip(conn->remote_addr());
        ret = RequestHandler(conn, buf, hr);
        if (ret < 0) { //connection closed
            return;
        }
        if (ret > 0) {
            Any context(new HttpRequest(std::move(hr)));
            conn->set_context(context);
            return;
        }
    }
}
}
}
