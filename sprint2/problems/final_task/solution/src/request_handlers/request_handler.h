#pragma once
#include "http_server.h"
#include "application.h"
#include "api_v1_request_handlers_executor.h"
#include "static_file_request_handlers_executor.h"

#include <filesystem>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using StringResponse = http::response<http::string_body>;
namespace fs = std::filesystem;
using namespace std::string_literals;

class RequestHandler {
public:
    explicit RequestHandler(app::Application& application, fs::path static_content_root_path, net::io_context& io)
        : application_{application}, static_content_root_path_{static_content_root_path},io_{io},strand_{net::make_strand(io_)} {
    }

    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /*
    # Обработчик отвратительный, есть идея как сделать его более гибким, но из-за шаблонов другая реализация пока не взлетела (надо обсудить).
    */
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {// 
        // Обработать запрос request и отправить ответ, используя send

        if(req.target().starts_with("/api/"s)){
            auto foo =[this, &req, send0=std::move(send)]()mutable {
                rh_storage::ApiV1RequestHandlerExecutor<http::request<Body, http::basic_fields<Allocator>>, Send>
                ::GetInstance()
                .Execute(req, application_, std::move(send0));
            };
            net::dispatch(strand_, foo);
            return;
        } else {
            bool b =rh_storage::StaticFileRequestHandlerExecutor<http::request<Body, http::basic_fields<Allocator>>, Send>
                ::GetInstance()
                .Execute(req, static_content_root_path_, std::move(send));
            if(b)
                return;
            else
                rh_storage::PageNotFoundHandler(req, application_, send);
        };
    }

private:
    app::Application& application_;
    fs::path static_content_root_path_;
    net::io_context& io_;
    Strand strand_;
    
};

}  // namespace http_handler
