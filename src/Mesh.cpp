#include "Mesh.h"

PipelineInfo Mesh::graphicspipeline = {};

Mesh::Mesh() {
	if (graphicspipeline.pipeline == VK_NULL_HANDLE) initGraphicsPipeline();
	initBuffers();
	heightmap = nullptr;
}

Mesh::Mesh(ImageInfo* h) : Mesh(){
	heightmap = h;
	initDescriptorSets();
}

Mesh::~Mesh() {
	terminateBuffers();
	terminateDescriptorSets();
}

void Mesh::initGraphicsPipeline() {
	graphicspipeline.stages = VK_SHADER_STAGE_VERTEX_BIT 
		| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT;
	graphicspipeline.topo = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	graphicspipeline.shaderfilepathprefix = "mesh";
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[3] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, 
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm)}
	};
	graphicspipeline.vertexinputstateci = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, 0,
		1, &vertinbindingdesc,
		3, &vertinattribdesc[0]
	};
	graphicspipeline.pushconstantrange = {
		VK_SHADER_STAGE_VERTEX_BIT
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		sizeof(MeshGraphicsPCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[1] {{
		0, // heightmap
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	graphicspipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1, &dslbindings[0]
	};
	graphicspipeline.depthtest = true;

	gh->createPipeline(graphicspipeline);
}

void Mesh::terminatePipelines() {
	gh->destroyPipeline(graphicspipeline);
}

void Mesh::initBuffers() {
	std::vector<Vertex> verttemp;
	verttemp.push_back({glm::vec3(-1, 0, -1), glm::vec2(0, 0)});
	verttemp.push_back({glm::vec3(-1, 0, 1), glm::vec2(0, 1)});
	verttemp.push_back({glm::vec3(1, 0, -1), glm::vec2(1, 0)});
	verttemp.push_back({glm::vec3(1, 0, 1), glm::vec2(1, 1)});

	for (auto& v : verttemp) {
		v.position *= 100;
	}

	generateAABB(verttemp);
	
	std::vector<Index> idxtemp;
	idxtemp.push_back(0);
	idxtemp.push_back(1);
	idxtemp.push_back(2);
	idxtemp.push_back(1);
	idxtemp.push_back(3);
	idxtemp.push_back(2);

	gh->createVertexAndIndexBuffers(vertexbuffer, indexbuffer, verttemp, idxtemp);
}

void Mesh::terminateBuffers() {
	gh->destroyBuffer(vertexbuffer);
	gh->destroyBuffer(indexbuffer);
}

void Mesh::initDescriptorSets() {
	VkDescriptorImageInfo di[1] {
		{heightmapsampler, heightmap->view, heightmap->layout}
	};
	gh->createDescriptorSet(
		graphicsdescriptorset, 
		graphicspipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		&di[0], nullptr);
}

void Mesh::terminateDescriptorSets() {
	gh->destroyDescriptorSet(graphicsdescriptorset);
}

void Mesh::recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	if (data.ds != VK_NULL_HANDLE) {
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline.pipeline);
		vkCmdPushConstants(
			cb,
			graphicspipeline.layout, 
			VK_SHADER_STAGE_VERTEX_BIT 
			| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
			| VK_SHADER_STAGE_FRAGMENT_BIT,
			0u,
			sizeof(MeshGraphicsPCData),
			data.pcdata);
		vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicspipeline.layout,
			0u, 1u, &data.ds,
			0u, nullptr);
		VkDeviceSize dummyoffset = 0u;
		vkCmdBindVertexBuffers(cb, 0, 1, &data.pVertexBuffer->buffer, &dummyoffset);
		vkCmdBindIndexBuffer(cb, data.pIndexBuffer->buffer, 0u, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(cb, data.pIndexBuffer->size / sizeof(Index), 1u, 0u, 0u, 0u);
	}
	else {
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline.pipeline);
		vkCmdPushConstants(
			cb,
			graphicspipeline.layout, 
			VK_SHADER_STAGE_VERTEX_BIT,
			0u,
			sizeof(MeshGraphicsPCData),
			data.pcdata);
		/*
		vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicspipeline.layout,
			0u, 1u, &data.ds,
			0u, nullptr);
			*/
		VkDeviceSize dummyoffset = 0u;
		vkCmdBindVertexBuffers(cb, 0, 1, &data.pVertexBuffer->buffer, &dummyoffset);
		vkCmdBindIndexBuffer(cb, data.pIndexBuffer->buffer, 0u, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(cb, data.pIndexBuffer->size / sizeof(Index), 1u, 0u, 0u, 0u);
	}
}

void Mesh::generateAABB(std::vector<Vertex> v) {
	AABB[0] = glm::vec3(INFINITY);
	AABB[1] = -glm::vec3(INFINITY);
	for (auto& vx : v) {
		for (uint8_t d = 0; d < 3; d++) {
			if (vx.position[d] < AABB[0][d]) AABB[0][d] = vx.position[d];
			if (vx.position[d] > AABB[1][d]) AABB[1][d] = vx.position[d];
		}
	}
}
