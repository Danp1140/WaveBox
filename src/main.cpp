#include "Ocean.h"

int main() {
	/*
	 * As we approach having multiple drawables (ocean floor, ui, etc.), below are some notes on better ways of doing things
	 * - we will have to have a running calculation of max possible ds allocs needed
	 */
	GH graphicshandler = GH();

	// I believe the below can be moved to a scene object
	Ocean ocean = Ocean(&graphicshandler);
	Camera camera = Camera();

	/*
	ocean.getPCDataPtr()->cameravp = glm::perspective<float>(0.74, 16./9., 0.01, 100.);
	ocean.getPCDataPtr()->cameravp[1][1] *= -1;
	ocean.getPCDataPtr()->cameravp *= glm::lookAt<float>(glm::vec3(3.), glm::vec3(0.), glm::vec3(0., 1., 0.));
	*/
	ocean.getGraphicsPCDataPtr()->cameravp = camera.getVP();
	ocean.getGraphicsPCDataPtr()->camerapos = camera.getCartesianPosition();
	cbRecData tempdata {
		ocean.getVertexBufferPtr(), 
		ocean.getIndexBufferPtr(), 
		reinterpret_cast<void *>(ocean.getGraphicsPCDataPtr()), 
		ocean.getGraphicsDescriptorSet()
	};
	std::vector<cbRecFunc> rectasks;
	rectasks.push_back(cbRecFunc([tempdata] (VkCommandBuffer& cb) {Ocean::recordGraphicsCommandBuffer(cb, tempdata);}));
	tempdata.pcdata = reinterpret_cast<void *>(ocean.getComputePCDataPtr());
	tempdata.ds = ocean.getComputeDescriptorSet();
	rectasks.push_back(cbRecFunc([tempdata] (VkCommandBuffer& cb) {Ocean::recordComputeCommandBuffer(cb, tempdata);}));
	while (!graphicshandler.endLoop()){
		ocean.getGraphicsPCDataPtr()->cameravp = camera.getVP();
		ocean.getGraphicsPCDataPtr()->camerapos = camera.getCartesianPosition();
		ocean.getComputePCDataPtr()->t = (float)glfwGetTime();
		// to clean up first draw, we could delay one frame before we draw anything, run compute once or smth
		// camera.addPhi(0.01);
		graphicshandler.loop(rectasks);
	}

	graphicshandler.waitIdle();
	Ocean::terminatePipelines();
	return 1;
}
