#include "UIHandler.h"

UIHandler::UIHandler(GH* g, VkCommandBuffer c) : gh(g), uicb(c) {
	PipelineInfo p;
	p.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	p.shaderfilepathprefix = "ui";
	p.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(UIPushConstantData)};
	gh->createPipeline(p);
	UIPipelineInfo uip;
	uip.layout = p.layout;
	uip.pipeline = p.pipeline;
	uip.dsl = p.dsl;
	UIComponent::setGraphicsPipeline(uip);

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
			VK_SHADER_STAGE_VERTEX_BIT,
			0u, sizeof(UIPushConstantData),
			c->getPCDataPtr());
		UIPushConstantData* p = c->getPCDataPtr();
		vkCmdDraw(cb, 6, 1, 0, 0);
	});

	UIRibbon* topribbon = new UIRibbon();
	topribbon->addOption(L"File");
	topribbon->setPos(vec2(0, 2240 - 100));
	topribbon->setExt(vec2(3584, 100));
	for (UIComponent* c : topribbon->getChildren()) {
		c->setPos(vec2(0, 2240 - 100));
		c->setExt(vec2(200, 100));
	}
	roots = {topribbon};
}

UIHandler::~UIHandler() {
	PipelineInfo p;
	p.layout = UIComponent::getGraphicsPipeline().layout;
	p.pipeline = UIComponent::getGraphicsPipeline().pipeline;
	p.dsl = UIComponent::getGraphicsPipeline().dsl;
	gh->destroyPipeline(p);
}

void UIHandler::draw() {
	for(UIComponent* c : roots) {
		c->draw();
	}
}
