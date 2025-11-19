#pragma once

#include <stack>
#include <glm/glm.hpp>
#include "entt/entt.hpp"
#include "relation.h"
#include "world.h"

namespace Engine {
    struct GlobalTransform {
        glm::mat4 val;
    }
    struct LocalTransform {
        glm::mat4 val;
    }

    void PropagateTransformsSystem(World world) {
        /*
        world.registry.sort<relationship>([&registry](const entt::entity lhs, const entt::entity rhs) {
            const auto &clhs = registry.get<relationship(lhs);
            const auto &crhs = registry.get<relationship>(rhs);
            return crhs.parent == lhs || clhs.next == rhs || (!(clhs.parent == rhs || crhs.next == lhs) && (clhs.parent < crhs.parent || (clhs.parent == crhs.parent && &clhs < &crhs)));
        });
        */

        auto view = world.registry.view<Relationship,LocalTransform>();

        std::stack stack;

        // push root entities to stack
        for(auto entity : view) {
            auto &relation = view.get<Relationship>(entity);
            auto &local = view.get<LocalTransform>(entity);
            
            if(relation.parent == entt::null) {                
                stack.push(entity);
                registry.emplace_or_replace<GlobalTransform>(entity, GlobalTransform{local.val});
            }
        }

        // Traverse hierarchy and propagate transforms
        while(!stack.empty()) {
            auto entity = stack.top();
            stack.pop();

            auto &parent_relation = view.get<Relationship>(entity);
            auto &parent_transform = view.get<GlobalTransform>(entity);

            auto current_child_entity = parent_relation.first;
            while(current_child_entity != entt::null) {
                auto &child_relation = view.get<Relationship>(current_child_entity);
                // if child has any children, push it to the stack to keep descending
                if child_relation.first != entt::null {
                    stack.push(current_child_entity);
                }

                auto& local = view.get<LocalTransform>(current_child_entity);
                registry.emplace_or_replace<GlobalTransform>(entity, GlobalTransform{parent_transform.val * local.val});

                current_child_entity = child_relation.next;
            }
        }
    }
}