#include "Scene.h"

int main() {
	GH graphicshandler = GH();

	Camera camera = Camera();

	Scene* s = new Scene(&graphicshandler, &camera);

	while (!graphicshandler.endLoop()){
		s->draw();
	}

	graphicshandler.waitIdle();
	Ocean::terminatePipelines();
	Mesh::terminatePipelines();
	Scene::terminatePipelines(&graphicshandler);
	Drawable::terminateSamplers();
	// TODO: fix so that we don't have destruction validation errors
	// okay so the issue im seeing is that the destructor is never called or double called, depending on the structure we try to use
	delete s;
	return 1;
}
