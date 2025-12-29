#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system; 

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог
    void OrderHotDog(HotDogHandler handler) {
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        
        auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(
            net::make_strand(io_));  // Упростил
        
        auto counter = std::make_shared<int>(0);
        auto hotdog_id = std::make_shared<int>(0);
        
        auto try_make_hotdog = [strand, handler, counter, hotdog_id, sausage, bread]() {
            if (*counter == 2) {
                try {
                    // Создаём хот-дог
                    HotDog hot_dog{++(*hotdog_id), sausage, bread};
                    net::post(*strand, [handler, hot_dog = std::move(hot_dog)]() mutable {
                        handler(Result<HotDog>{std::move(hot_dog)});
                    });
                } catch (...) {
                    net::post(*strand, [handler]() {
                        handler(Result<HotDog>::FromCurrentException());
                    });
                }
            }
        };
        
        // Готовим сосиску
        sausage->StartFry(*gas_cooker_, [sausage, strand, counter, try_make_hotdog]() {
            auto timer = std::make_shared<net::steady_timer>(*strand);
            timer->expires_after(std::chrono::milliseconds(1500));
            
            timer->async_wait([sausage, strand, counter, try_make_hotdog, timer]
                            (const sys::error_code& ec) {  
                if (ec) return;
                
                try {
                    sausage->StopFry();
                    (*counter)++;
                    net::post(*strand, try_make_hotdog);
                } catch (...) {
                    (*counter)++;
                    net::post(*strand, try_make_hotdog);
                }
            });
        });
        
        // Готовим булку
        bread->StartBake(*gas_cooker_, [bread, strand, counter, try_make_hotdog]() {
            auto timer = std::make_shared<net::steady_timer>(*strand);
            timer->expires_after(std::chrono::milliseconds(1000));
            
            timer->async_wait([bread, strand, counter, try_make_hotdog, timer]
                            (const sys::error_code& ec) {  
                if (ec) return;
                
                try {
                    bread->StopBaking();
                    (*counter)++;
                    net::post(*strand, try_make_hotdog);
                } catch (...) {
                    (*counter)++;
                    net::post(*strand, try_make_hotdog);
                }
            });
        });
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};