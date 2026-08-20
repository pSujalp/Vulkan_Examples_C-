#pragma once
#include <ufbx.h>
#include <vector>
#include "vk_mesh.h"
#include <string>
#include <stdio.h>
#include <assert.h>
#include <unordered_map>



class FBX_Model_Loader
{
public:
    std::vector<std::pair<Mesh, std::vector<uint32_t>>> Meshes;
    std::unordered_map<ufbx_mesh*,uint16_t> mesh_element_id;
    ufbx_scene *scene;
    FBX_Model_Loader() {}
    FBX_Model_Loader(const std::string &filepath);


};
