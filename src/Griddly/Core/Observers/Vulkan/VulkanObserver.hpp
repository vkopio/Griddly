#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace vk {
class VulkanDevice;
class VulkanInstance;
struct VulkanRenderContext;
}  // namespace vk

namespace griddly {


struct VulkanObserverConfig {
  uint32_t tileSize;
  std::string imagePath;
  std::string shaderPath;
};


class VulkanObserver : public Observer {
 public:
  VulkanObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig observerConfig);

  ~VulkanObserver();

  void print(std::shared_ptr<uint8_t> observation) override;

  void init(ObserverConfig observerConfig) override;

  void resetRenderSurface();

  void release() override;

 protected:
  std::unique_ptr<vk::VulkanDevice> device_;
  VulkanObserverConfig vulkanObserverConfig_;

  uint32_t pixelWidth_;
  uint32_t pixelHeight_;

 private:
  static std::shared_ptr<vk::VulkanInstance> instance_;
};

}  // namespace griddly