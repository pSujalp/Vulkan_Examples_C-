#include "FBX_Loader.h"

FBX_Loader::FBX_Loader(std::string filepath)
{
    scene = ufbx_load_file(filepath.c_str(), NULL, NULL);
    if (!scene)
    {
        printf("Failed to load FBX file: %s\n", filepath.c_str());
        return;
    }

    for (ufbx_mesh *mesh : scene->meshes)
    {
        std::vector<uint32_t> tri_indices;
        tri_indices.resize(mesh->max_face_triangles * 3);

        for (ufbx_mesh_part &part : mesh->material_parts)
        {
            ufbx_material *material = NULL;
            if (part.index < mesh->materials.count)
            {
                material = mesh->materials.data[part.index];
            }
            printf(". [%u] material: %s\n", part.index,
                   material ? material->name.data : "(none)");

            // One mesh per material part, built up across all its faces.
            Mesh meshy;

            for (uint32_t face_index : part.face_indices)
            {
                ufbx_face face = mesh->faces[face_index];
                uint32_t num_tris = ufbx_triangulate_face(
                    tri_indices.data(), tri_indices.size(), mesh, face);

                size_t base = meshy._vertices.size();
                meshy._vertices.resize(base + num_tris * 3);
                meshy._indices.resize(base + num_tris * 3);

                for (size_t i = 0; i < num_tris * 3; i++)
                {
                    uint32_t index = tri_indices[i];

                    meshy._vertices[base + i].position = glm::vec3{
                        mesh->vertex_position[index].x,
                        mesh->vertex_position[index].y,
                        mesh->vertex_position[index].z};

                    meshy._vertices[base + i].normal = glm::vec3{
                        mesh->vertex_normal[index].x,
                        mesh->vertex_normal[index].y,
                        mesh->vertex_normal[index].z};

                    meshy._vertices[base + i].uv = glm::vec2{
                        mesh->vertex_uv[index].x,
                        mesh->vertex_uv[index].y};

                    meshy._indices[base + i] = static_cast<uint32_t>(base + i);
                }
            }
            Meshes.push_back({meshy, tri_indices});
        }
    }
}