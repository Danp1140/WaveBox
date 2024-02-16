#include "GraphicsHandler.h"

#define HEIGHT_MAP_RESOLUTION 2048

typedef struct OceanGraphicsPCData {
	glm::mat4 cameravp;
	alignas(16) glm::vec3 camerapos;
} OceanGraphicsPCData;

typedef struct OceanComputePCData {
	float t;
} OceanComputePCData;

// in GLSL, we can use aliasing to access different kinds of waves accurately!
typedef enum WaveType {
	WAVE_TYPE_LINEAR
} WaveType;

typedef enum DepthType {
	DEPTH_TYPE_CONSTANT,
	DEPTH_TYPE_MAPPED
} DepthType;

typedef struct LinearWaveData {
	float H, k, omega, d;

	LinearWaveData(float height, float length, float depth) {
		H = height;
		k = 6.28 / length;
		d = depth;
		omega = sqrt(9.8 * k * tanh(k * d));
	}
} LinearWaveData;

typedef struct Wave {
	WaveType wavetype;
	DepthType depthtype;
	union {
		LinearWaveData linear;
	};

	Wave(LinearWaveData l, DepthType d) {
		linear = l;
		wavetype = WAVE_TYPE_LINEAR;
		depthtype = d;
	}
} Wave;



/* 
 * if/when we implement any other kind of object that is rendered, we should probably have a superclass that ensures
 * things like having a pipeline & command buffer recording functions.
 */
class Ocean {
public:
	static PipelineInfo graphicspipeline, computepipeline;

	Ocean(GH* g);
	~Ocean();

	BufferInfo * getVertexBufferPtr() {return &vertexbuffer;}
	BufferInfo * getIndexBufferPtr() {return &indexbuffer;}
	OceanGraphicsPCData * getGraphicsPCDataPtr() {return &graphicspcdata;}
	OceanComputePCData * getComputePCDataPtr() {return &computepcdata;}
	VkDescriptorSet getGraphicsDescriptorSet() {return graphicsdescriptorset;}
	VkDescriptorSet getComputeDescriptorSet() {return computedescriptorset;}

	// Also inits heightmapsampler
	static void initGraphicsPipeline();
	static void initComputePipeline();
	static void terminatePipelines();
	static void recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);

private:
	static GH* gh;
	static VkSampler heightmapsampler;
	BufferInfo vertexbuffer, indexbuffer, wavebuffer;
	ImageInfo heightmap;
	OceanGraphicsPCData graphicspcdata;
	OceanComputePCData computepcdata;
	VkDescriptorSet graphicsdescriptorset, computedescriptorset;

	std::vector<Wave> waves;

	void initBuffers();
	void terminateBuffers();
	void initDescriptorSets();
	void terminateDescriptorSets();
};
