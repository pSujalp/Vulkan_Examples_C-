#pragma once
#include <ufbx.h>
#include <vector>
#include "vk_mesh.h"
#include <string>
#include <stdio.h>
#include <assert.h>





class FBX_Model_Loader
{
public:
    std::vector<std::pair<Mesh, std::vector<uint32_t>>> Meshes;
    ufbx_scene *scene;
    FBX_Model_Loader() {}
    FBX_Model_Loader(const std::string &filepath);


};
