// [file name]: json_converter.cpp
#include "json_converter.h"
#include <sstream>
#include <boost/json.hpp>

namespace jsonConverter{

namespace json = boost::json;

std::string ConvertMapListToJson(const model::Game& game){
    json::array root;
    
    for(const auto& item : game.GetMaps()){
        json::object map;
        map["id"] = *item.GetId();  // Используем operator*() для Tagged типа
        map["name"] = item.GetName();
        root.push_back(std::move(map));
    }
    
    return json::serialize(root);
}

void AddRoadsToJson(const model::Map& map, json::object& root){
    json::array roads;
    for(const auto& item : map.GetRoads()){
        json::object road;
        road["x0"] = item.GetStart().x;
        road["y0"] = item.GetStart().y;
        if(item.IsHorizontal()) {
            road["x1"] = item.GetEnd().x;
        } else {
            road["y1"] = item.GetEnd().y;
        }
        roads.push_back(std::move(road));
    }
    root["roads"] = std::move(roads);
}

void AddBuildingsToJson(const model::Map& map, json::object& root){
    json::array buildings;
    for(const auto& item : map.GetBuildings()){
        json::object building;
        building["x"] = item.GetBounds().position.x;
        building["y"] = item.GetBounds().position.y;
        building["w"] = item.GetBounds().size.width;
        building["h"] = item.GetBounds().size.height;
        buildings.push_back(std::move(building));
    }
    root["buildings"] = std::move(buildings);
}

void AddOfficesToJson(const model::Map& map, json::object& root){
    json::array offices;
    for(const auto& item : map.GetOffices()){
        json::object office;
        office["id"] = *item.GetId();  // Используем operator*() для Tagged типа
        office["x"] = item.GetPosition().x;
        office["y"] = item.GetPosition().y;
        office["offsetX"] = item.GetOffset().dx;
        office["offsetY"] = item.GetOffset().dy;
        offices.push_back(std::move(office));
    }
    root["offices"] = std::move(offices);
}

std::string ConvertMapToJson(const model::Map& map){
    json::object root;
    
    root["id"] = *map.GetId();  // Используем operator*() для Tagged типа
    root["name"] = map.GetName();
    
    AddRoadsToJson(map, root);
    AddBuildingsToJson(map, root);
    AddOfficesToJson(map, root);
    
    return json::serialize(root);
}

std::string CreateMapNotFoundResponse(){
    json::object root;
    root["code"] = "mapNotFound";
    root["message"] = "Map not found";
    return json::serialize(root);
}

std::string CreateBadRequestResponse(){
    json::object root;
    root["code"] = "badRequest";
    root["message"] = "Bad request";
    return json::serialize(root);
}

std::string CreatePageNotFoundResponse(){
    json::object root;
    root["code"] = "pageNotFound";
    root["message"] = "Page not found";
    return json::serialize(root);
}

}  // namespace jsonConverter