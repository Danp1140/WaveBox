#include "GraphicsHandler.h"

class Ocean {
public:
	Ocean();
	~Ocean();

	static void initPipeline();
	static void terminatePipeline();

private:
	static PipelineInfo pipeline;
};
