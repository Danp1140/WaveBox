#include "Ocean.h"

int main() {
	GH graphicshandler = GH();

	while (!graphicshandler.endLoop()){
		graphicshandler.loop();
	}

	return 1;
}
