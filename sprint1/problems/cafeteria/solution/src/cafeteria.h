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
        // Уникальный ID для этого заказа
        static std::atomic<int> next_hotdog_id{1};
        int hotdog_id = next_hotdog_id++;
        
        // Получаем ингредиенты
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        
        // Создаём strand для этого заказа
        auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(
            net::make_strand(io_));
        
        // Состояние для этого заказа
        struct OrderState {
            bool sausage_ready = false;
            bool bread_ready = false;
            std::optional<Result<HotDog>> result;
            int hotdog_id;
        };
        
        auto state = std::make_shared<OrderState>();
        state->hotdog_id = hotdog_id;
        
        // Функция для создания хот-дога, когда оба ингредиента готовы
        auto try_make_hotdog = [strand, handler, state, sausage, bread]() {
            if (state->sausage_ready && state->bread_ready && !state->result) {
                try {
                    // Создаём хот-дог
                    HotDog hot_dog{state->hotdog_id, sausage, bread};
                    state->result = Result<HotDog>{std::move(hot_dog)};
                    net::post(*strand, [handler, result = *state->result]() mutable {
                        handler(std::move(result));
                    });
                } catch (const std::exception& e) {
                    state->result = Result<HotDog>{std::make_exception_ptr(e)};
                    net::post(*strand, [handler, result = *state->result]() mutable {
                        handler(std::move(result));
                    });
                } catch (...) {
                    state->result = Result<HotDog>::FromCurrentException();
                    net::post(*strand, [handler, result = *state->result]() mutable {
                        handler(std::move(result));
                    });
                }
            }
        };
        
        // Готовим сосиску
        try {
            sausage->StartFry(*gas_cooker_, [sausage, strand, state, try_make_hotdog]() {
                auto timer = std::make_shared<net::steady_timer>(*strand);
                timer->expires_after(std::chrono::milliseconds(1500));
                
                timer->async_wait([sausage, strand, state, try_make_hotdog, timer]
                                (const sys::error_code& ec) {
                    if (ec) return;
                    
                    try {
                        sausage->StopFry();
                        state->sausage_ready = true;
                        net::post(*strand, try_make_hotdog);
                    } catch (...) {
                        state->sausage_ready = true;
                        net::post(*strand, try_make_hotdog);
                    }
                });
            });
        } catch (...) {
            // Если не удалось начать готовить сосиску, сразу возвращаем ошибку
            net::post(*strand, [handler]() {
                handler(Result<HotDog>::FromCurrentException());
            });
            return;
        }
        
        // Готовим булку
        try {
            bread->StartBake(*gas_cooker_, [bread, strand, state, try_make_hotdog]() {
                auto timer = std::make_shared<net::steady_timer>(*strand);
                timer->expires_after(std::chrono::milliseconds(1000));
                
                timer->async_wait([bread, strand, state, try_make_hotdog, timer]
                                (const sys::error_code& ec) {
                    if (ec) return;
                    
                    try {
                        bread->StopBaking();
                        state->bread_ready = true;
                        net::post(*strand, try_make_hotdog);
                    } catch (...) {
                        state->bread_ready = true;
                        net::post(*strand, try_make_hotdog);
                    }
                });
            });
        } catch (...) {
            // Если не удалось начать готовить булку, нужно попытаться освободить сосиску
            net::post(*strand, [handler]() {
                handler(Result<HotDog>::FromCurrentException());
            });
            return;
        }
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};