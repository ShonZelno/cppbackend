#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>
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

    void OrderHotDog(HotDogHandler handler) {
        static std::atomic<int> next_hotdog_id{1};
        int hotdog_id = next_hotdog_id++;
        
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        
        auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(
            net::make_strand(io_));
        
        struct OrderState {
            std::atomic<bool> sausage_ready{false};
            std::atomic<bool> bread_ready{false};
            std::atomic<bool> completed{false};
            int hotdog_id;
        };
        
        auto state = std::make_shared<OrderState>();
        state->hotdog_id = hotdog_id;
        
        // Функция для создания хот-дога
        auto try_make_hotdog = [strand, handler, state, sausage, bread]() {
            bool sausage_ready = state->sausage_ready.load();
            bool bread_ready = state->bread_ready.load();
            bool expected = false;
            
            if (sausage_ready && bread_ready && 
                state->completed.compare_exchange_strong(expected, true)) {
                
                try {
                    // Создаём хот-дог
                    HotDog hot_dog{state->hotdog_id, sausage, bread};
                    net::post(*strand, [handler, hot_dog = std::move(hot_dog)]() mutable {
                        handler(Result<HotDog>{std::move(hot_dog)});
                    });
                } catch (const std::exception& e) {
                    net::post(*strand, [handler, e]() {
                        handler(Result<HotDog>{std::make_exception_ptr(e)});
                    });
                } catch (...) {
                    net::post(*strand, [handler]() {
                        handler(Result<HotDog>::FromCurrentException());
                    });
                }
            }
        };
        
        // Обработчик ошибок
        auto on_error = [strand, handler](const std::string& error_msg) {
            net::post(*strand, [handler, error_msg]() {
                handler(Result<HotDog>{std::make_exception_ptr(
                    std::runtime_error(error_msg))});
            });
        };
        
        // Готовим сосиску
        try {
            sausage->StartFry(*gas_cooker_, [sausage, strand, state, try_make_hotdog]() {
                auto timer = std::make_shared<net::steady_timer>(*strand);
                timer->expires_after(std::chrono::milliseconds(1500));
                
                timer->async_wait([sausage, strand, state, try_make_hotdog, timer]
                                (const sys::error_code& ec) {
                    if (ec) {
                        return;
                    }
                    
                    try {
                        sausage->StopFry();
                        state->sausage_ready.store(true);
                        net::post(*strand, try_make_hotdog);
                    } catch (const std::exception& e) {
                        state->sausage_ready.store(true);
                        net::post(*strand, try_make_hotdog);
                    }
                });
            });
        } catch (const std::exception& e) {
            on_error(std::string("Failed to start frying sausage: ") + e.what());
            return;
        } catch (...) {
            on_error("Failed to start frying sausage: unknown error");
            return;
        }
        
        // Готовим булку
        try {
            bread->StartBake(*gas_cooker_, [bread, strand, state, try_make_hotdog]() {
                auto timer = std::make_shared<net::steady_timer>(*strand);
                timer->expires_after(std::chrono::milliseconds(1000));
                
                timer->async_wait([bread, strand, state, try_make_hotdog, timer]
                                (const sys::error_code& ec) {
                    if (ec) {
                        return;
                    }
                    
                    try {
                        bread->StopBaking();
                        state->bread_ready.store(true);
                        net::post(*strand, try_make_hotdog);
                    } catch (const std::exception& e) {
                        state->bread_ready.store(true);
                        net::post(*strand, try_make_hotdog);
                    }
                });
            });
        } catch (const std::exception& e) {
            on_error(std::string("Failed to start baking bread: ") + e.what());
            return;
        } catch (...) {
            on_error("Failed to start baking bread: unknown error");
            return;
        }
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};