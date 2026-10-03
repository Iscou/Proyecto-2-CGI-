#pragma once
#include<string.h>
#include "Mesh.h"

namespace util {
	std::vector<float> load_model_from_file(const char* filename, glm::mat4 preTransform);
}

class ObjLoader {
public:
	static Mesh cargarOBJ(int idObjeto, const std::string& rutaArchivo);
};