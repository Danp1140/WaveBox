#include "Drawable.h"

typedef struct MeshGraphicsPCData {
	glm::mat4 cameravp;
	alignas(16) glm::vec3 camerapos;
} MeshGraphicsPCData;

class Mesh : public Drawable {
public:
	glm::vec3 AABB[2]; // [0] => min, [1] => max

	Mesh();
	Mesh(ImageInfo* h);
	~Mesh();

	static void terminatePipelines();

	MeshGraphicsPCData* getGraphicsPCDataPtr() {return &graphicspcdata;}

	static void recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data);

private:
	static PipelineInfo graphicspipeline;
	MeshGraphicsPCData graphicspcdata;
	ImageInfo* heightmap;

	static void initGraphicsPipeline();
	void initBuffers();
	void terminateBuffers();
	void initDescriptorSets();
	void terminateDescriptorSets();


	void generateAABB(std::vector<Vertex> v);
};
