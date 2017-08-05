#ifndef ATLAS_BACKEND_H
#define ATLAS_BACKEND_H

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
// GPUOpen memory allocator
#define VMA_STATS_STRING_ENABLED 0
#include <string.h>
#include "vk_mem_alloc.h"

namespace Atlas {
    namespace Backend {
        static void log(const std::string& message);
        static void warning(const std::string& message);
        static void error(const std::string& message);
        bool validate(VkResult result);

        enum VendorID {
            VK_VENDOR_ID_AMD = 0x1002,
            VK_VENDOR_ID_IMGTEC = 0x1010,
            VK_VENDOR_ID_NVIDIA = 0x10DE,
            VK_VENDOR_ID_ARM = 0x13B5,
            VK_VENDOR_ID_QUALCOMM = 0x5143,
            VK_VENDOR_ID_INTEL = 0x8086
        };

        enum QueueFamily {
            QUEUE_FAMILY_UNIVERSAL = 0,
            QUEUE_FAMILY_COMPUTE,
            QUEUE_FAMILY_TRANSFER,
            QUEUE_FAMILY_COUNT,
            QUEUE_FAMILY_FIRST = QUEUE_FAMILY_UNIVERSAL
        };

        struct PhysicalDevice {
            VkPhysicalDevice device;
            VkPhysicalDeviceProperties props;
            VkPhysicalDeviceFeatures features;

            struct {
                union {
                    struct { uint32_t universal, transfer, compute; };
                    uint32_t indices[QUEUE_FAMILY_COUNT];
                };
                uint32_t universal_count, transfer_count, compute_count;
            } queue_families;
            std::unordered_set<std::string> supported_extensions;
        };

        struct Instance {
            // Sets data; does not create the instance
            Instance(const std::string& app_name, uint32_t app_version = VK_MAKE_VERSION(0,0,0));
            ~Instance();
            bool init();

            VkApplicationInfo app_info;
            VkInstanceCreateFlags instance_flags;
            // If any extensions here are not global, the layer which provides them
            // will be automatically be loaded, if it exists
            std::vector<const char*> enabled_extensions;

            inline VkInstance vk() const {
                return m_instance;
            }
            inline uint32_t get_n_physical_devices() const {
                return m_physical_devices.size();
            }
            uint32_t get_preferred_device_index() const;
            inline const PhysicalDevice& get_physical_device(uint32_t index) const {
                return m_physical_devices[index];
            }

            bool is_extension_supported(const std::string& name) const;
            bool is_layer_supported(const std::string& name) const;
            void load_func_pointers();
            static VkBool32 default_dbg_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);
        protected:
            friend class Device;

            PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
            PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
            PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

            VkDebugReportCallbackEXT m_dbg_callback;
            std::vector<VkLayerProperties> m_supported_layers;
            std::vector<const char*> m_enabled_layers;

            // map extension names to the names of the layers which provide them
            // doubles as a lookup of supported extensions
            struct ExtensionProvider {
                const char* layer_name;
                uint32_t spec_version;
            };
            std::unordered_map<std::string, ExtensionProvider> m_extension_providers;


            VkInstance m_instance;
            std::vector<PhysicalDevice> m_physical_devices;
        };

        struct Device {
            // TODO: use VK_KHX_device_group_creation for multi-GPU?
            Device(const Backend::Instance& source);
            Device(const Backend::Instance& source, uint32_t physical_device_index);
            ~Device();
            bool init();

            VkCommandPoolCreateFlags command_pool_flags;
            uint32_t n_threads;

            // For any required extension, push_back to here before calling init()
            std::vector<const char*> enabled_extensions;
            // For any optional extension, only push_back if this returns true
            bool is_extension_supported(const std::string& name) const;
            VkCommandPool get_command_pool(QueueFamily family, uint32_t thread_index = 0);
        protected:
            std::unordered_set<std::string> m_supported_extensions;
            VkInstance m_instance;
            const PhysicalDevice& m_physical_device;
            const std::vector<const char*>& m_enabled_layers;
            VkDevice m_device;
            VmaAllocator m_allocator;

            std::vector<VkCommandPool> m_command_pools;
            VkQueue m_universal_queue, m_compute_queue, m_transfer_queue;
            static constexpr uint32_t compute_owns_self = 1;
            static constexpr uint32_t compute_is_dedicated = 1 << 1;
            static constexpr uint32_t transfer_owns_self = 1 << 2;
            static constexpr uint32_t transfer_is_dedicated = 1 << 3;
            uint32_t m_queue_flags;
        };
    }
}

#endif // ATLAS_BACKEND_H