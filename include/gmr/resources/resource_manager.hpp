#ifndef GMR_RESOURCE_MANAGER_HPP
#define GMR_RESOURCE_MANAGER_HPP

#include "gmr/types.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace gmr {

template<typename HandleType, typename ResourceType>
class ResourceManager {
public:
    virtual ~ResourceManager() = default;
    
    HandleType load(const std::string& path) {
        // Check if already loaded
        auto it = path_to_handle_.find(path);
        if (it != path_to_handle_.end()) {
            return it->second;
        }
        
        // Load new resource
        auto resource = load_resource(path);
        if (!resource) {
            return static_cast<HandleType>(INVALID_HANDLE);
        }
        
        HandleType handle = static_cast<HandleType>(resources_.size());
        resources_.push_back(std::move(*resource));
        path_to_handle_[path] = handle;
        
        return handle;
    }
    
    ResourceType* get(HandleType handle) {
        if (handle < 0 || handle >= static_cast<HandleType>(resources_.size())) {
            return nullptr;
        }
        return &resources_[handle];
    }
    
    const ResourceType* get(HandleType handle) const {
        if (handle < 0 || handle >= static_cast<HandleType>(resources_.size())) {
            return nullptr;
        }
        return &resources_[handle];
    }
    
    bool valid(HandleType handle) const {
        return handle >= 0 && handle < static_cast<HandleType>(resources_.size());
    }
    
    void clear() {
        for (auto& resource : resources_) {
            unload_resource(resource);
        }
        resources_.clear();
        path_to_handle_.clear();
    }
    
    size_t count() const { return resources_.size(); }
    
protected:
    virtual std::optional<ResourceType> load_resource(const std::string& path) = 0;
    virtual void unload_resource(ResourceType& resource) = 0;
    
private:
    std::vector<ResourceType> resources_;
    std::unordered_map<std::string, HandleType> path_to_handle_;
};

} // namespace gmr

#endif
