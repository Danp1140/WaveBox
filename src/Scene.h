#include "Ocean.h"

typedef struct EnvGraphicsPCData {
	glm::mat4 cameravp;
} EnvGraphicsPCData;

class Scene {
public:
	Scene(GH* graphicshandler, Camera* c);
	~Scene(); 

	void draw();

	static void terminatePipelines(GH* g);
	static void recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data);

private:
	GH* gh;
	Camera* cam;
	Ocean ocean;
	cbRecFunc* recfuncs;
	double lastt, dt;
	static PipelineInfo skyboxgraphicspipeline;
	static VkSampler skyboxsampler;
	EnvGraphicsPCData envpcdata;
	ImageInfo envmap;
	VkDescriptorSet envdescriptorset;

	void record();

	void updatePCs();

	void initEnvMap();
	void terminateEnvMap();

	void initDescriptorSets();
	void terminateDescriptorSets();
};
