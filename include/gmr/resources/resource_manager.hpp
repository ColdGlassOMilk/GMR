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
            // Resource already cached - increment reference count
            HandleType handle = it->second;
            if (handle >= 0 && handle < static_cast<HandleType>(ref_counts_.size())) {
                ref_counts_[handle]++;
            }
            return handle;
        }

        // Load new resource
        auto resource = load_resource(path);
        if (!resource) {
            return static_cast<HandleType>(INVALID_HANDLE);
        }

        HandleType handle = static_cast<HandleType>(resources_.size());
        resources_.push_back(std::move(*resource));
        ref_counts_.push_back(1);  // Initial refcount = 1
        handle_to_path_[handle] = path;
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

    // Release a reference to a resource. If refcount reaches 0, unload it.
    void release(HandleType handle) {
        if (handle < 0 || handle >= static_cast<HandleType>(ref_counts_.size())) {
            return;
        }

        // Decrement reference count
        if (ref_counts_[handle] > 0) {
            ref_counts_[handle]--;

            // If refcount reaches 0, unload the resource
            if (ref_counts_[handle] == 0) {
                unload_resource(resources_[handle]);

                // Remove from path cache
                auto path_it = handle_to_path_.find(handle);
                if (path_it != handle_to_path_.end()) {
                    path_to_handle_.erase(path_it->second);
                    handle_to_path_.erase(path_it);
                }

                // Note: We don't remove from resources_ vector to keep handle indices stable.
                // This means handles are not reused, but it prevents dangling handle issues.
                // For full cleanup, use clear().
            }
        }
    }

    // Add a reference to an existing resource (useful for copying handles)
    void add_ref(HandleType handle) {
        if (handle >= 0 && handle < static_cast<HandleType>(ref_counts_.size())) {
            ref_counts_[handle]++;
        }
    }

    // Get current reference count for a resource
    int get_ref_count(HandleType handle) const {
        if (handle >= 0 && handle < static_cast<HandleType>(ref_counts_.size())) {
            return ref_counts_[handle];
        }
        return 0;
    }

    void clear() {
        for (size_t i = 0; i < resources_.size(); ++i) {
            if (ref_counts_[i] > 0) {
                unload_resource(resources_[i]);
            }
        }
        resources_.clear();
        ref_counts_.clear();
        path_to_handle_.clear();
        handle_to_path_.clear();
    }

    size_t count() const { return resources_.size(); }
    
protected:
    virtual std::optional<ResourceType> load_resource(const std::string& path) = 0;
    virtual void unload_resource(ResourceType& resource) = 0;

private:
    std::vector<ResourceType> resources_;
    std::vector<int> ref_counts_;  // Reference count for each resource
    std::unordered_map<std::string, HandleType> path_to_handle_;  // Path -> Handle lookup
    std::unordered_map<HandleType, std::string> handle_to_path_;  // Handle -> Path lookup (for cleanup)
};

} // namespace gmr

#endif
