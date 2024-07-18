#include "UIHandler.h"

UIHandler::UIHandler(GH* g, VkCommandBuffer c, GLFWwindow* w) : gh(g), uicb(c), window(w) {
	UIComponent::setScreenExtent(vec2(gh->getScreenExtent().width, gh->getScreenExtent().height));
	glfwSetWindowUserPointer(window, reinterpret_cast<void*>(this));
	glfwSetCursorPosCallback(window, [] (GLFWwindow* w, double x, double y) {
		UIHandler* self = reinterpret_cast<UIHandler*>(glfwGetWindowUserPointer(w));
		for (UIComponent* c : self->roots) {
			c->listenMousePos(glfwCoordsToPixels(w, x, y), nullptr);
		}
	});
	glfwSetMouseButtonCallback(window, [] (GLFWwindow* w, int but, int act, int mod) {
		UIHandler* self = reinterpret_cast<UIHandler*>(glfwGetWindowUserPointer(w));
		for (UIComponent* c : self->roots) {
			c->listenMouseClick(act == GLFW_PRESS, nullptr);
		}
	});

	PipelineInfo p;
	p.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	p.shaderfilepathprefix = "ui";
	p.pushconstantrange = {
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
		0u, 
		sizeof(UIPushConstantData)
	};
	VkDescriptorSetLayoutBinding dslbindings[1] {{
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	p.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1, &dslbindings[0]
	};
	gh->createPipeline(p);
	UIPipelineInfo uip;
	uip.layout = p.layout;
	uip.pipeline = p.pipeline;
	uip.dsl = p.dsl;
	UIComponent::setGraphicsPipeline(uip);

	ImageInfo i;
	i.format = VK_FORMAT_R8_UNORM;
	i.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	i.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	i.extent = {1, 1};
	gh->createImage(i);
	UIImageInfo uii {
		i.image,
		i.memory,
		i.view,
		i.extent,
		i.layout
	};
	UIComponent::setNoTex(uii);

	gh->createSampler(textsampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	VkDescriptorSet d;
	VkDescriptorImageInfo ii = {textsampler, uii.view, uii.layout};
	gh->createDescriptorSet(
		d,
		uip.dsl, 
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 
		&ii, nullptr);
	UIComponent::setDefaultDS(d);

	void* data = reinterpret_cast<void*>(&uicb);
	UIComponent::setDefaultDrawFunc([data] (UIComponent* c) {
		VkCommandBuffer cb = *reinterpret_cast<VkCommandBuffer*>(data);
		vkCmdBindPipeline(
			cb, 
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			UIComponent::getGraphicsPipeline().pipeline);
		vkCmdPushConstants(
			cb,
			UIComponent::getGraphicsPipeline().layout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0u, sizeof(UIPushConstantData),
			c->getPCDataPtr());
		vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			UIComponent::getGraphicsPipeline().layout,
			0u, 1u, c->getDSPtr(),
			0u, nullptr);
		vkCmdDraw(cb, 6, 1, 0, 0);
	});
	UIText::setTexLoadFunc([this] (UIText* self, unorm* d) {
		ImageInfo i;
		i.image = self->getTexPtr()->image;
		i.memory = self->getTexPtr()->memory;
		i.view = self->getTexPtr()->view;
		i.format = VK_FORMAT_R8_UNORM;
		// would like for this to actually just be sampled bit and for layout to be shader read only optimal
		i.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		i.layout = VK_IMAGE_LAYOUT_GENERAL;
		i.extent = self->getTexPtr()->extent;
		bool updating = i.image != VK_NULL_HANDLE;
		if (updating) {
			gh->waitIdle();
			gh->destroyImage(i);
		}
		gh->createImage(i);
		std::cout << "created img " << i.image << std::endl;
		std::wcout << L"for text " << self->getText() << std::endl;
		self->getTexPtr()->image = i.image;
		self->getTexPtr()->memory = i.memory;
		self->getTexPtr()->view = i.view;
		self->getTexPtr()->layout = i.layout;

		gh->updateImage(i, reinterpret_cast<void*>(d));

		VkDescriptorImageInfo ii = {textsampler, self->getTexPtr()->view, self->getTexPtr()->layout};
		UIPipelineInfo uipipeline = UIComponent::getGraphicsPipeline();

		if (updating) {
			gh->updateDescriptorSet(
				*(self->getDSPtr()),
				0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				ii, {});
		}
		else {
			gh->createDescriptorSet(
				*(self->getDSPtr()),
				uipipeline.dsl,
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
				&ii, nullptr);
		}
	});
	UIText::setTexDestroyFunc([this] (UIText* self) {
		if (self->getTexPtr()->image == VK_NULL_HANDLE) return;
		ImageInfo i;
		i.image = self->getTexPtr()->image;
		i.memory = self->getTexPtr()->memory;
		i.view = self->getTexPtr()->view;
		std::cout << "destroyed image " << i.image << std::endl;
		gh->destroyImage(i);
		self->getTexPtr()->image = VK_NULL_HANDLE;
		self->getTexPtr()->memory = VK_NULL_HANDLE;
		self->getTexPtr()->view = VK_NULL_HANDLE;
	});
}

UIHandler::~UIHandler() {
	PipelineInfo p;
	p.layout = UIComponent::getGraphicsPipeline().layout;
	p.pipeline = UIComponent::getGraphicsPipeline().pipeline;
	p.dsl = UIComponent::getGraphicsPipeline().dsl;
	gh->destroyPipeline(p);
	ImageInfo i;
	i.image = UIComponent::getNoTex().image;
	i.memory = UIComponent::getNoTex().memory;
	i.view = UIComponent::getNoTex().view;
	gh->destroyImage(i);
	gh->destroySampler(textsampler);
}

void UIHandler::draw() {
	for(UIComponent* c : roots) {
		c->draw();
	}
}

vec2 UIHandler::glfwCoordsToPixels(GLFWwindow* w, double x, double y) {
	int sizex, sizey, fbsizex, fbsizey;
	glfwGetWindowSize(w, &sizex, &sizey);
	glfwGetFramebufferSize(w, &fbsizex, &fbsizey);
	vec2 result = vec2(x, y) / vec2(sizex, sizey);
	result.y *= -1;
	result.y += 1;
	return result * vec2(fbsizex, fbsizey);
}
