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

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
void OrderHotDog(HotDogHandler handler) {
    // Получаем ингредиенты
    auto sausage = store_.GetSausage();
    auto bread = store_.GetBread();
    
    // Создаём strand для безопасного доступа
    auto strand = std::make_shared<net::strand<net::io_context::executor_type>>(
        net::make_strand(io_.get_executor()));
    
    // Счётчик готовых ингредиентов
    auto counter = std::make_shared<int>(0);
    auto hotdog_id = std::make_shared<int>(0);
    
    // Функция для создания хот-дога, когда оба ингредиента готовы
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
        // Запускаем таймер на 1.5 секунды
        auto timer = std::make_shared<net::steady_timer>(
            strand->context(),
            std::chrono::milliseconds(1500)
        );
        
        timer->async_wait([sausage, strand, counter, try_make_hotdog, timer]
                          (sys::error_code ec) {
            if (ec) return;
            
            try {
                sausage->StopFry();
                (*counter)++;
                net::post(*strand, try_make_hotdog);
            } catch (...) {
                // В случае ошибки просто увеличиваем счётчик
                (*counter)++;
                net::post(*strand, try_make_hotdog);
            }
        });
    });
    
    // Готовим булку
    bread->StartBake(*gas_cooker_, [bread, strand, counter, try_make_hotdog]() {
        // Запускаем таймер на 1 секунду
        auto timer = std::make_shared<net::steady_timer>(
            strand->context(),
            std::chrono::milliseconds(1000)
        );
        
        timer->async_wait([bread, strand, counter, try_make_hotdog, timer]
                          (sys::error_code ec) {
            if (ec) return;
            
            try {
                bread->StopBaking();
                (*counter)++;
                net::post(*strand, try_make_hotdog);
            } catch (...) {
                // В случае ошибки просто увеличиваем счётчик
                (*counter)++;
                net::post(*strand, try_make_hotdog);
            }
        });
    });
}

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
