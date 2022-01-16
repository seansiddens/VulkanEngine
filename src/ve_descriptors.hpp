#pragma once

#include "ve_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace ve {

class VeDescriptorSetLayout {
   public:
    class Builder {
       public:
        Builder(VeDevice &veDevice) : veDevice{veDevice} {}

        Builder &addBinding(uint32_t binding,
                            VkDescriptorType descriptorType,
                            VkShaderStageFlags stageFlags,
                            uint32_t count = 1);
        std::unique_ptr<VeDescriptorSetLayout> build() const;

       private:
        VeDevice &veDevice;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };

    VeDescriptorSetLayout(VeDevice &veDevice,
                          std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~VeDescriptorSetLayout();
    VeDescriptorSetLayout(const VeDescriptorSetLayout &) = delete;
    VeDescriptorSetLayout &operator=(const VeDescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

   private:
    VeDevice &veDevice;
    VkDescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

    friend class VeDescriptorWriter;
};

class VeDescriptorPool {
   public:
    class Builder {
       public:
        Builder(VeDevice &veDevice) : veDevice{veDevice} {}

        Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder &setMaxSets(uint32_t count);
        std::unique_ptr<VeDescriptorPool> build() const;

       private:
        VeDevice &veDevice;
        std::vector<VkDescriptorPoolSize> poolSizes{};
        uint32_t maxSets = 1000;
        VkDescriptorPoolCreateFlags poolFlags = 0;
    };

    VeDescriptorPool(VeDevice &veDevice,
                     uint32_t maxSets,
                     VkDescriptorPoolCreateFlags poolFlags,
                     const std::vector<VkDescriptorPoolSize> &poolSizes);
    ~VeDescriptorPool();
    VeDescriptorPool(const VeDescriptorPool &) = delete;
    VeDescriptorPool &operator=(const VeDescriptorPool &) = delete;

    bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout,
                            VkDescriptorSet &descriptor) const;

    void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

    void resetPool();

   private:
    VeDevice &veDevice;
    VkDescriptorPool descriptorPool;

    friend class VeDescriptorWriter;
};

class VeDescriptorWriter {
   public:
    VeDescriptorWriter(VeDescriptorSetLayout &setLayout, VeDescriptorPool &pool);

    VeDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
    VeDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

    bool build(VkDescriptorSet &set);
    void overwrite(VkDescriptorSet &set);

   private:
    VeDescriptorSetLayout &setLayout;
    VeDescriptorPool &pool;
    std::vector<VkWriteDescriptorSet> writes;
};

}  // namespace ve