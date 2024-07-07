#include "Scene.h"

PipelineInfo Scene::skyboxgraphicspipeline = {};
VkSampler Scene::skyboxsampler = VK_NULL_HANDLE;

Scene::Scene(GH* graphicshandler, Camera* c) : ocean(Ocean(graphicshandler)){
	gh = graphicshandler;

	if (skyboxgraphicspipeline.pipeline == VK_NULL_HANDLE) {
		skyboxgraphicspipeline.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		skyboxgraphicspipeline.shaderfilepathprefix = "env";
		skyboxgraphicspipeline.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(EnvGraphicsPCData)};
		VkDescriptorSetLayoutBinding dslbindings[1] {{
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		}};
		skyboxgraphicspipeline.descsetlayoutci = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1, &dslbindings[0]
		};
		gh->createPipeline(skyboxgraphicspipeline);

		gh->createSampler(skyboxsampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	}

	cam = c;
	cam->update(gh->primarywindow, 0);
	updatePCs();

	initEnvMap();
	initDescriptorSets();

	recfuncs = nullptr;
	record();
	lastt = glfwGetTime();

}

Scene::~Scene() {
	terminateDescriptorSets();
	terminateEnvMap();
	// for (uint8_t i = 0; i < 3; i++) recfuncs[i].~function();
	// delete[] recfuncs;
}

void Scene::draw() {
	dt = glfwGetTime() - lastt;
	lastt = dt + lastt;
	cam->update(gh->primarywindow, dt);
	updatePCs();
	gh->loop(recfuncs);
	recfuncs[4] = nullptr;
	// std::cout << (1.f / dt) << std::endl;
}

void Scene::terminatePipelines(GH* g) {
	g->destroySampler(skyboxsampler);
	g->destroyPipeline(skyboxgraphicspipeline);
}

void Scene::recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxgraphicspipeline.pipeline);
	vkCmdPushConstants(
		cb,
		skyboxgraphicspipeline.layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0u,
		sizeof(EnvGraphicsPCData),
		data.pcdata);
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		skyboxgraphicspipeline.layout,
		0u, 1u, &data.ds,
		0u, nullptr);
	vkCmdDraw(cb, 36u, 1u, 0u, 0u);
}

void Scene::record() {
	if (recfuncs) delete[] recfuncs;
	recfuncs = new cbRecFunc[6];
	cbRecData data {
		ocean.floor->getVertexBufferPtr(), 
		ocean.floor->getIndexBufferPtr(), 
		reinterpret_cast<void *>(ocean.getComputePCDataPtr()), 
		ocean.getPropertyComputeDescriptorSet()
	};
	recfuncs[4] = cbRecFunc([data] (VkCommandBuffer& cb) {Ocean::recordPropertyComputeCommandBuffer(cb, data);});
	data.ds = ocean.getComputeDescriptorSet();
	recfuncs[1] = cbRecFunc([data] (VkCommandBuffer& cb) {Ocean::recordComputeCommandBuffer(cb, data);});
	data.ds = ocean.getShoalingComputeDescriptorSet();
	data.pcdata = reinterpret_cast<void *>(ocean.getShoalingComputePCDataPtr());
	recfuncs[5] = cbRecFunc([data] (VkCommandBuffer& cb) {Ocean::recordShoalingComputeCommandBuffer(cb, data);});
	data.pcdata = reinterpret_cast<void *>(&envpcdata);
	data.ds = envdescriptorset;
	recfuncs[3] = cbRecFunc([data] (VkCommandBuffer& cb) {Scene::recordGraphicsCommandBuffer(cb, data);});
	data.pcdata = reinterpret_cast<void *>(ocean.floor->getGraphicsPCDataPtr());
	data.ds = ocean.floor->getGraphicsDescriptorSet();
	recfuncs[2] = cbRecFunc([data] (VkCommandBuffer& cb) {Mesh::recordGraphicsCommandBuffer(cb, data);});
	data.pVertexBuffer = ocean.getVertexBufferPtr();
	data.pIndexBuffer = ocean.getIndexBufferPtr();
	data.pcdata = reinterpret_cast<void *>(ocean.getGraphicsPCDataPtr());
	data.ds = ocean.getGraphicsDescriptorSet();
	recfuncs[0] = cbRecFunc([data] (VkCommandBuffer& cb) {Ocean::recordGraphicsCommandBuffer(cb, data);});
}

void Scene::updatePCs() {
	ocean.getGraphicsPCDataPtr()->cameravp = cam->getVP();
	ocean.getGraphicsPCDataPtr()->camerapos = cam->getCartesianPosition();
	ocean.getComputePCDataPtr()->t = (float)glfwGetTime();
	ocean.getShoalingComputePCDataPtr()->t = (float)glfwGetTime();
	ocean.floor->getGraphicsPCDataPtr()->cameravp = cam->getVP();
	ocean.floor->getGraphicsPCDataPtr()->camerapos = cam->getCartesianPosition();
	envpcdata.cameravp = cam->getEnvVP();
}

void Scene::initEnvMap() {
	envmap.extent = {1024, 1024};
	envmap.format = VK_FORMAT_R8G8B8A8_SRGB;
	// ideally we wouldn't keep transfer dst, itd be gotten rid of with a layout transition
	envmap.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// envmap.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	envmap.layout = VK_IMAGE_LAYOUT_GENERAL;
	envmap.viewtype = VK_IMAGE_VIEW_TYPE_CUBE;
	envmap.numlayers = 6;
	gh->createImage(envmap);

	/*
	glm::vec4* temp = new glm::vec4[128 * 128 * 6];
	for (uint32_t i = 0; i < 128 * 128 * 6; i++) {
		temp[i] = glm::vec4((float)i / 128. / 128. / 6.);
	}
	gh->updateImage(envmap, reinterpret_cast<void*>(temp));
	delete[] temp;
	*/
	
	uint32_t* temp = new uint32_t[1024 * 1024 * 6];
	memset(temp, 0, 1024 * 1024 * 6);
	for (uint8_t layer = 0; layer < 6; layer++) {
		png_image image;
		memset(&image, 0, sizeof(image));
		image.version = PNG_IMAGE_VERSION;
		std::string skyboxfilepath = WORKING_DIRECTORY "/resources/envmaps/skyonfire/";
		if (layer == 0) skyboxfilepath += "px.png";
		else if (layer == 1) skyboxfilepath += "nx.png";
		else if (layer == 2) skyboxfilepath += "py.png";
		else if (layer == 3) skyboxfilepath += "ny.png";
		else if (layer == 4) skyboxfilepath += "pz.png";
		else if (layer == 5) skyboxfilepath += "nz.png";


		png_image_begin_read_from_file(&image, skyboxfilepath.c_str());

		png_bytep buffer;
		image.format = PNG_FORMAT_RGBA;
		buffer = reinterpret_cast<png_bytep>(malloc(PNG_IMAGE_SIZE(image)));

		png_image_finish_read(&image, NULL, buffer, 0, NULL);

		for (uint32_t i = 0; i < 1024 * 1024; i++) {
			//temp[i] = ImageInfo::uint32rgba8FromVec4(glm::vec4((float)i / 128. / 128. /6., 0, 0, 1));
			temp[i + 1024 * 1024 * layer] = uint32_t(buffer[4 * i])
				| (uint32_t(buffer[4 * i + 1]) << 8)
				| (uint32_t(buffer[4 * i + 2]) << 16)
				| (uint32_t(buffer[4 * i + 3]) << 24);
			//temp[i] = ImageInfo::uint32rgba8FromVec4(glm::vec4(1, 1, 0, 1));
			
			/*temp[i] = uint32_t(0xff000000 * (float)i / 128. / 128. / 6.)
				| 0x00ff0000 
				| 0x0000ff00 
				| 0x000000ff;
				*/
			// temp[i] = 0xff0000ff;
		}
		gh->updateImage(envmap, reinterpret_cast<void*>(temp));
		delete[] buffer;
		png_image_free(&image);
	}
	delete[] temp;

	gh->updateDescriptorSet(
		ocean.getGraphicsDescriptorSet(), 
		2, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		{skyboxsampler, envmap.view, envmap.layout}, {});
}

void Scene::terminateEnvMap() {
	gh->destroyImage(envmap);
}

void Scene::initDescriptorSets() {
	VkDescriptorImageInfo ii {
		skyboxsampler, envmap.view, envmap.layout
	};
	gh->createDescriptorSet(
		envdescriptorset,
		skyboxgraphicspipeline.dsl,
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		&ii, nullptr);
}

void Scene::terminateDescriptorSets() {
	gh->destroyDescriptorSet(envdescriptorset);
}
