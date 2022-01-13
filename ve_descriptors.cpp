#include "ve_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>

namespace ve {

// *************** Descriptor Set Layout Builder *********************

VeDescriptorSetLayout::Builder &VeDescriptorSetLayout::Builder::addBinding(
    uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
    uint32_t count) {
    assert(bindings.count(binding) == 0 && "Binding already in use");
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    bindings[binding] = layoutBinding;
    return *this;
}

std::unique_ptr<VeDescriptorSetLayout> VeDescriptorSetLayout::Builder::build() const {
    return std::make_unique<VeDescriptorSetLayout>(veDevice, bindings);
}

// *************** Descriptor Set Layout *********************

VeDescriptorSetLayout::VeDescriptorSetLayout(
    VeDevice &veDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : veDevice{veDevice}, bindings{bindings} {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
    for (auto kv : bindings) {
        setLayoutBindings.push_back(kv.second);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(veDevice.device(), &descriptorSetLayoutInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

VeDescriptorSetLayout::~VeDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(veDevice.device(), descriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************

VeDescriptorPool::Builder &VeDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType,
                                                                  uint32_t count) {
    poolSizes.push_back({descriptorType, count});
    return *this;
}

VeDescriptorPool::Builder &VeDescriptorPool::Builder::setPoolFlags(
    VkDescriptorPoolCreateFlags flags) {
    poolFlags = flags;
    return *this;
}
VeDescriptorPool::Builder &VeDescriptorPool::Builder::setMaxSets(uint32_t count) {
    maxSets = count;
    return *this;
}

std::unique_ptr<VeDescriptorPool> VeDescriptorPool::Builder::build() const {
    return std::make_unique<VeDescriptorPool>(veDevice, maxSets, poolFlags, poolSizes);
}

// *************** Descriptor Pool *********************

VeDescriptorPool::VeDescriptorPool(VeDevice &veDevice, uint32_t maxSets,
                                   VkDescriptorPoolCreateFlags poolFlags,
                                   const std::vector<VkDescriptorPoolSize> &poolSizes)
    : veDevice{veDevice} {
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = maxSets;
    descriptorPoolInfo.flags = poolFlags;

    if (vkCreateDescriptorPool(veDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VeDescriptorPool::~VeDescriptorPool() {
    vkDestroyDescriptorPool(veDevice.device(), descriptorPool, nullptr);
}

bool VeDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout,
                                          VkDescriptorSet &descriptor) const {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;

    // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
    // a new pool whenever an old pool fills up. But this is beyond our current scope
    if (vkAllocateDescriptorSets(veDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void VeDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
    vkFreeDescriptorSets(veDevice.device(), descriptorPool,
                         static_cast<uint32_t>(descriptors.size()), descriptors.data());
}

void VeDescriptorPool::resetPool() { vkResetDescriptorPool(veDevice.device(), descriptorPool, 0); }

// *************** Descriptor Writer *********************

VeDescriptorWriter::VeDescriptorWriter(VeDescriptorSetLayout &setLayout, VeDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

VeDescriptorWriter &VeDescriptorWriter::writeBuffer(uint32_t binding,
                                                    VkDescriptorBufferInfo *bufferInfo) {
    assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(bindingDescription.descriptorCount == 1 &&
           "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = bufferInfo;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}

VeDescriptorWriter &VeDescriptorWriter::writeImage(uint32_t binding,
                                                   VkDescriptorImageInfo *imageInfo) {
    assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(bindingDescription.descriptorCount == 1 &&
           "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = imageInfo;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}

bool VeDescriptorWriter::build(VkDescriptorSet &set) {
    bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
    if (!success) {
        return false;
    }
    overwrite(set);
    return true;
}

void VeDescriptorWriter::overwrite(VkDescriptorSet &set) {
    for (auto &write : writes) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(pool.veDevice.device(), writes.size(), writes.data(), 0, nullptr);
}

}  // namespace ve
