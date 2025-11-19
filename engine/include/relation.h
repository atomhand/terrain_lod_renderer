#pragma once

#include <entt/entt.hpp>

namespace Engine {
    // follows pattern suggested by Skypjack https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
    struct Relationship {
        std::size_t children{};
        entt::entity first{entt::null};
        entt::entity prev{entt::null};
        entt::entity next{entt::null};
        entt::entity parent{entt::null};
    }

    void set_parent(entt::entity entity, entt::entity parent, entt::registry registry) {
        


        auto &parent_relation = registry.get<Relationship>(parent);

        Relationship child_relation;
        child_relation.parent = parent;

        if parent_relation.first == entt::null {
            parent_relation.first = new_child;
            registry.emplace<Relationship>(entity, child_relation);
        }
        
    }
}