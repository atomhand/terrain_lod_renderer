#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <iostream>
#include <concepts>

namespace Engine {
    class SceneNode {
    private:
        SceneNode & operator=(const SceneNode&) = delete;
        SceneNode(const SceneNode&) = delete;
    public:
        glm::mat4 localTransform = glm::mat4(1.f);
        glm::mat4 globalTransform;
        SceneNode* parent = nullptr;
        std::vector<SceneNode*> children;
        bool enabled = true;

        SceneNode() {};
        virtual ~SceneNode() {};
    };
    template <typename T>
    concept DerivedFromSceneNode = std::derived_from<T,SceneNode>;

    class SceneGraph {
    private:
        bool SubtreeContains(SceneNode* subtree, SceneNode* target) {
            if(subtree == target) {
                return true;
            }
            for(auto child : subtree->children) {
                if(SubtreeContains(child,target)) {
                    return true;
                }
            }
            return false;
        }

        void PropagateTransformsRecursive(SceneNode* parent) {
            for(auto child : parent->children) {
                child->globalTransform = parent->globalTransform * child->localTransform;
                PropagateTransformsRecursive(child);
            }
        }

        void DeleteNodeRecursive(SceneNode* node) {
            for(auto child : node->children) {
                DeleteNodeRecursive(child);
            }
            delete node;
        }

        template<DerivedFromSceneNode T> void FilterRecursive(SceneNode* subtree, std::vector<T*>& result) {
            if(!subtree->enabled)
                return;
            
            if(T* t= dynamic_cast<T*>(subtree); t != nullptr) {
                result.push_back(t);
            }
            for(auto child : subtree->children) {
                FilterRecursive(child, result);
            }
        }
        
        SceneGraph & operator=(const SceneGraph&) = delete;
        SceneGraph(const SceneGraph&) = delete;
    public:    
        SceneNode* root;

        bool SetParent(SceneNode* item, SceneNode* newparent) {
            if(item == root) {                
                std::cout << "Error - trying to give scene root a parent";
                return false;
            }
            // do not create cycles in the scene tree
            if(newparent != nullptr && SubtreeContains(item,newparent)) {
                std::cout << "Error - attempted to make cyclic relationship in scene graph";
                return false;
            }
            // remove former parent
            SceneNode* oldparent = item->parent;
            if(oldparent != nullptr) {
                if(newparent == oldparent) {
                    return false;
                }
                // erase child pointer from parent's children list
                for(auto it = oldparent->children.begin(); it != oldparent->children.end(); ++it) {
                    if(*it == item) {
                        oldparent->children.erase(it);
                        break;
                    }
                }
            }
            item->parent = newparent;
            if(newparent != nullptr)
                newparent->children.push_back(item);
            return true;
        }

        void PropagateTransforms() {
            root->globalTransform = root->localTransform;
            PropagateTransformsRecursive(root);
        }

        // Delete node (as well as all of its descendant nodes)
        void DeleteNode(SceneNode* node) {
            if(node == root) {
                std::cout << "Deleting scenegraph root node";
            } else {            
                // remove parent
                SetParent(node, nullptr);
            }

            DeleteNodeRecursive(node);
        }

        template<DerivedFromSceneNode T> std::vector<T*> Filter(SceneNode* subtree) {
            std::vector<T*> result;
            FilterRecursive<T>(subtree,result);
            return result;
        }
        template<DerivedFromSceneNode T> std::vector<T*> Filter() {
            return Filter<T>(root);
        }

        SceneGraph() {
            root = new SceneNode();
            root->localTransform = glm::mat4(1.f);
        }
        ~SceneGraph() {
            DeleteNode(root);
        }
    };
}