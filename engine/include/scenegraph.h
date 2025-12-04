#pragma once
#include <vector>
#include <iostream>
#include <concepts>
#include <cassert>
#include <stack>
#include <queue>
#include <glm/glm.hpp>

namespace Engine {
    class World;
    class SceneGraph;

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

        // This function is called every frame for each node
        virtual void Update(World& world) {};

        // Called when a node is added to the scene for the first time
        virtual void OnEnter(SceneGraph& sceneGraph) {};

        SceneNode() {};
        virtual ~SceneNode() {};
    };
    template <typename T>
    concept DerivedFromSceneNode = std::derived_from<T,SceneNode>;

    class SceneGraph {
    private:
        // Returns true if the subtree starting at parent contains
        // the target node
        bool SubtreeContains(SceneNode* parent, SceneNode* target) {
            std::stack<SceneNode*> toProcess;
            toProcess.push(parent);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    if(child == target)
                        return true;
                    toProcess.push(child);
                }                
            }

            return false;
        }
        
        SceneGraph & operator=(const SceneGraph&) = delete;
        SceneGraph(const SceneGraph&) = delete;
    public:    
        SceneNode* root;

        // Sets the parent of item to newparent, updating
        // the newparent's list of children, as well as clearing
        // the child from their old parent's list if they have one
        bool SetParent(SceneNode* item, SceneNode* newparent) {
            assert(item != root); // Cannot give the scene root a parent
            assert(item != nullptr);
            SceneNode* oldparent = item->parent;
            if(newparent == oldparent) {
                return false;
            }
            // do not create cycles in the scene tree
            if(newparent != nullptr && SubtreeContains(item,newparent)) {
                std::cout << "Error - attempted to make cyclic relationship in scene graph";
                return false;
            }
            
            // remove former parent
            item->parent = newparent;
            if(newparent != nullptr)
                newparent->children.push_back(item);
            
            if(oldparent != nullptr) {
                // erase child pointer from parent's children list
                for(auto it = oldparent->children.begin(); it != oldparent->children.end(); ++it) {
                    if(*it == item) {
                        oldparent->children.erase(it);
                        break;
                    }
                }
            } else {
                // If there was no previous parent, called init on the new child
                item->OnEnter(*this);
            }

            return true;
        }

        // Call the update function on all scene nodes, skipping disabled nodes and their descendants
        void NodeTickUpdate(World& world) {
            root->Update(world);
            // Depth first traversal
            std::stack<SceneNode*> toProcess;
            toProcess.push(root);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    if(child->enabled) {
                        child->Update(world);
                        if(child->children.size() > 0)
                            toProcess.push(child);
                    }
                }                
            }
        }

        // Propagate transforms down the scene hierarchy
        void PropagateTransforms() {
            root->globalTransform = root->localTransform;

            // Depth first traversal
            std::stack<SceneNode*> toProcess;
            toProcess.push(root);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    child->globalTransform = parent->globalTransform * child->localTransform;
                    if(child->children.size() > 0)
                        toProcess.push(child);
                }                
            }
        }

        // Delete node (as well as all of its descendant nodes)
        void DeleteNode(SceneNode* node) {
            assert(node != nullptr); // Deleting a null node is not valid
            if(node == root) {
                std::cout << "Deleting scenegraph root node";
            } else {            
                // remove parent
                SetParent(node, nullptr);
            }

            // Depth first traversal
            std::stack<SceneNode*> toProcess;
            toProcess.push(node);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    toProcess.push(child);
                }
                delete parent;            
            }
        }

        // Returns a list of nodes in the subtree that match the derived type T
        // Disabled nodes and their children are skipped
        template<DerivedFromSceneNode T> std::vector<T*> Filter(SceneNode* subtree) {
            std::vector<T*> result;
            
            // Depth first traversal
            std::stack<SceneNode*> toProcess;
            toProcess.push(subtree);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    if(child->enabled) {
                        if(T* t= dynamic_cast<T*>(child); t != nullptr)
                            result.push_back(t);
                        if(child->children.size() > 0)
                            toProcess.push(child);
                    }
                }         
            }

            return result;
        }

        // Returns a list of nodes in the graph that match the derived type T
        template<DerivedFromSceneNode T> std::vector<T*> Filter() {
            return Filter<T>(root);
        }

        // Returns the first node in the subtree that matches derived type T
        // (or nullptr if there is none)
        // Breadth first traversal
        template<DerivedFromSceneNode T> T* First(SceneNode* subtree) {            
            // Depth first traversal
            std::queue<SceneNode*> toProcess;
            toProcess.push(subtree);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.front();
                toProcess.pop();

                for(auto child : parent->children) {
                    if(child->enabled) {
                        if(T* t= dynamic_cast<T*>(child); t != nullptr)
                            return t;
                        if(child->children.size() > 0)
                            toProcess.push(child);
                    }
                }         
            }

            return nullptr;
        }

        // Returns the first node in the scene that matches derived type T
        // (or nullptr if there is none)
        // Breadth first traversal
        template<DerivedFromSceneNode T> T* First() {
            return First<T>(root);
        }

        // Returns a list of all nodes
        std::vector<SceneNode*> AllNodes() {
            std::vector<SceneNode*> result{root};
            // Depth first traversal
            std::stack<SceneNode*> toProcess;
            toProcess.push(root);
            while(!toProcess.empty()) {
                SceneNode* parent = toProcess.top();
                toProcess.pop();

                for(auto child : parent->children) {
                    if(child->enabled) {
                        result.push_back(child);
                        if(child->children.size() > 0)
                            toProcess.push(child);
                    }
                }         
            }

            return result;
        }

        SceneGraph() {
            root = new SceneNode();
        }
        ~SceneGraph() {
            DeleteNode(root);
        }
    };
}