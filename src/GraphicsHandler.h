#ifndef GRAPHICS_HANDLER_H
#define GRAPHICS_HANDLER_H
#include "Camera.h"

#include <iostream>
#include <fstream>

#include <vulkan/vk_enum_string_helper.h>
#include <libpng/png.h>

// #define VERBOSE_BUFFER_CREATION

#define GH_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define GH_MAX_FRAMES_IN_FLIGHT 6
#define WORKING_DIRECTORY "/Users/danp/Desktop/C Coding/WaveBox/"
#define NUM_SHADER_STAGES_SUPPORTED 5
	const VkShaderStageFlagBits supportedshaderstages[NUM_SHADER_STAGES_SUPPORTED] = {
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT
	};
	const char* const shaderstagestrs[NUM_SHADER_STAGES_SUPPORTED] = {
		"comp",
		"vert",
		"tesc",
		"tese",
		"frag"
	};

typedef struct BufferInfo {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size = -1u;
	VkBufferUsageFlags usage = 0u;
	VkMemoryPropertyFlags memprops = 0u;
} BufferInfo;

typedef struct ImageInfo {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkExtent2D extent = {0, 0};
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags usage = 0u;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageType type = VK_IMAGE_TYPE_2D;
	VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D;
	uint8_t numlayers = 1;

	size_t pixelSize() {
		switch (format) {
			case VK_FORMAT_D32_SFLOAT:
				return 4u;
			case VK_FORMAT_R8G8B8A8_SRGB:
				return 4u;
			default:
				return -1u;
		}
	}

	static uint32_t uint32rgba8FromVec4(glm::vec4 color) {
		return uint32_t((float)0xff000000 * color.a)
			| uint32_t((float)0x00ff0000 * color.b)
			| uint32_t((float)0x0000ff00 * color.g)
			| uint32_t((float)0x000000ff * color.r);
	}
} ImageInfo;

typedef struct PipelineInfo {
	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
	VkShaderStageFlags stages = 0u;
	const char* shaderfilepathprefix = nullptr;
	VkDescriptorSetLayoutCreateInfo descsetlayoutci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	VkPushConstantRange pushconstantrange = {};
	VkPipelineVertexInputStateCreateInfo vertexinputstateci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	bool depthtest = false;
	VkSpecializationInfo specinfo = {};
	VkPrimitiveTopology topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkExtent2D extent = {0, 0}; // in createPipeline, this is defaulted to the swapchain extent if left zeroed
	VkCullModeFlags cullmode = VK_CULL_MODE_BACK_BIT;
	VkRenderPass renderpass = VK_NULL_HANDLE; // if not set, will default to primaryrenderpass
} PipelineInfo;

typedef struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 norm;
} Vertex;

static const VkCommandBufferBeginInfo interimcommandbufferbi {
	VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	nullptr,
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	nullptr
};

typedef uint16_t Index;

typedef struct cbRecData {
	BufferInfo* pVertexBuffer = nullptr, * pIndexBuffer = nullptr;
	void* pcdata = nullptr;
	VkDescriptorSet ds = VK_NULL_HANDLE;
} cbRecData;

typedef std::function<void (VkCommandBuffer&)> cbRecFunc;

class GH {
public:
	GLFWwindow* primarywindow;
	VkDevice logicaldevice; // made public to enable certain VK function calls in other classes

	GH();
	~GH();

	void loop(cbRecFunc* rectasks);

	// This seems pretty ham-fisted...
	void waitIdle() {vkQueueWaitIdle(genericqueue);}

	bool endLoop() {return glfwWindowShouldClose(primarywindow);}

	void createPipeline(PipelineInfo& pi);
	void destroyPipeline(PipelineInfo& pi);
	void createDescriptorSet(
		VkDescriptorSet& ds,
		VkDescriptorSetLayout& dsl,
		std::vector<VkDescriptorType> type,
		VkDescriptorImageInfo* ii,
		VkDescriptorBufferInfo* bi);
	void updateDescriptorSet(
		VkDescriptorSet ds,
		uint32_t index,
		VkDescriptorType type,
		VkDescriptorImageInfo ii,
		VkDescriptorBufferInfo bi);
	void destroyDescriptorSet(VkDescriptorSet& ds);
	void createSampler(
		VkSampler& s,
		VkFilter filter,
		VkSamplerAddressMode addrmode);
	void destroySampler(VkSampler& s);
	void createBuffer(BufferInfo& bi);
	void destroyBuffer(BufferInfo& bi);
	void createVertexAndIndexBuffers(
		BufferInfo& vbi,
		BufferInfo& ibi,
		const std::vector<Vertex>& vertices,
		const std::vector<Index>& indices);
	/*
	 * Creates image & image view and allocates memory. Non-default values for all other members should be set
	 * in i before calling createImage.
	 */
	void createImage(ImageInfo& i);
	void destroyImage(ImageInfo& i);	

	void updateHostCoherentBuffer(BufferInfo& bi, void* data);
	void updateImage(ImageInfo& ii, void* data);
	void submitInterimCB();

	VkCommandBuffer getInterimCommandBuffer() {return interimcommandbuffer;}

private:
	VkInstance instance;
	VkSurfaceKHR primarysurface;
	VkDebugUtilsMessengerEXT debugmessenger;
	VkPhysicalDevice physicaldevice;
	VkQueue genericqueue;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchainextent; // TODO: eliminate this, get directly from scis
	uint8_t fifindex, queuefamilyindex;
	uint32_t sciindex, numsci;
	ImageInfo* swapchainimages, depthbuffer;
	VkRenderPass primaryrenderpass;
	VkFramebuffer* framebuffers;
	VkCommandPool commandpool;
	VkCommandBuffer* primarycommandbuffers, interimcommandbuffer;
	VkDescriptorPool descriptorpool;
	// TODO: re-check which of these are necessary after getting a bare-bones draw loop finished
	VkSemaphore* imageacquiredsemaphores,* submitfinishedsemaphores;
	VkFence* submitfinishedfences;
	const VkClearValue primaryclears[2] = {{0.1, 0.1, 0.1, 1.}, 1.};
	BufferInfo interimbuffer;

	/*
	 * Below are several graphics initialization functions. Most have self-explanatory names and are relatively
	 * simple and so are not worth exhaustively commenting on. Generally, those with names create_____ or 
	 * destroy_____ could reasonably be called multiple times. However, those with names init______ or 
	 * terminate_______ should only be called once per GH, likely in the constructor and destructor respectively.
	 * Some functions have sparse comments for things they do that are not self-explanatory.
	 */
	// TODO: Ensure createWindow checks if glfw is initialized before calling glfwInit()
	void createWindow(GLFWwindow*& w);
	void destroyWindow(GLFWwindow*& w);
	// init/terminateVulkanInstance also handle the primary surface & window, as they will always exist for a GH
	// TODO: Remove hard-coding and enhance initVulkanInstance's ability to dynamically enable needed extensions.
	void initVulkanInstance();
	void terminateVulkanInstance();
	static VkResult createDebugMessenger(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* createinfo,
		const VkAllocationCallbacks* allocator,
		VkDebugUtilsMessengerEXT* debugutilsmessenger);
	static void destroyDebugMessenger(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugutilsmessenger,
		const VkAllocationCallbacks* allocator);
	void initDebug();
	void terminateDebug();
	// TODO: As in initVulkanInstance, remove hard-coding and dynamically find best extensions, queue families, 
	// and hardware to use
	void initDevicesAndQueues();
	void terminateDevicesAndQueues();
	void initSwapchain();
	void terminateSwapchain();
	void initRenderpasses();
	void terminateRenderpasses();
	void initDepthBuffer();
	void terminateDepthBuffer();
	void initFramebuffers();
	void terminateFramebuffers();
	// Due to low total draw numbers, for now we will only need one thread and so one of each pool
	void initCommandPools();
	void terminateCommandPools();
	void initCommandBuffers();
	void terminateCommandBuffers();
	void initDescriptorPoolsAndSetLayouts();
	void terminateDescriptorPoolsAndSetLayouts();
	void initSyncObjects();
	void terminateSyncObjects();

	void createShader(
		VkShaderStageFlags stages,
		const char** filepaths,
		VkShaderModule** modules,
		VkPipelineShaderStageCreateInfo** createinfos,
		VkSpecializationInfo* specializationinfos);
	void destroyShader(VkShaderModule shader);

	void allocateDeviceMemory(
		const VkBuffer& buffer,
		const VkImage& image,
		VkDeviceMemory& memory,
		VkMemoryPropertyFlags memprops);
	void freeDeviceMemory(VkDeviceMemory& memory);

	void copyBufferToBuffer(BufferInfo& src, BufferInfo& dst);
	void copyBufferToImage(BufferInfo& src, ImageInfo& dst);

	void recordPrimaryCommandBuffer(cbRecFunc* rectasks);
	void submitAndPresent();

	static VKAPI_ATTR VkBool32 VKAPI_CALL validationCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callbackdata,
			void* userdata);
};
#endif
