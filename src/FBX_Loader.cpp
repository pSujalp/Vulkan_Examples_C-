#include "FBX_Loader.h"
#include <cassert>

FBX_Loader::FBX_Loader(const std::string &filepath)
{
    scene = ufbx_load_file(filepath.c_str(), NULL, NULL);
    if (!scene)
    {
        printf("Failed to load FBX file: %s\n", filepath.c_str());
        return;
    }

    std::vector<uint32_t> tri_indices;

    for (ufbx_mesh *mesh : scene->meshes)
    {
        printf("mesh '%s'\n", mesh->name.data);

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

            std::vector<Vertex> vertices;
            vertices.reserve(part.num_triangles * 3);

            for (uint32_t face_index : part.face_indices)
            {
                ufbx_face face = mesh->faces[face_index];
                uint32_t num_tris = ufbx_triangulate_face(
                    tri_indices.data(), tri_indices.size(), mesh, face);

                for (size_t i = 0; i < num_tris * 3; i++)
                {
                    uint32_t index = tri_indices[i];

                    Vertex v{};
                    v.position = glm::vec3{
                        mesh->vertex_position[index].x,
                        mesh->vertex_position[index].y,
                        mesh->vertex_position[index].z};
                    v.normal = glm::vec3{
                        mesh->vertex_normal[index].x,
                        mesh->vertex_normal[index].y,
                        mesh->vertex_normal[index].z};
                    v.uv = glm::vec2{
                        mesh->vertex_uv[index].x,
                        mesh->vertex_uv[index].y};

                    vertices.push_back(v);
                }
            }
            assert(vertices.size() == part.num_triangles * 3);

            ufbx_vertex_stream streams[1] = {
                {vertices.data(), vertices.size(), sizeof(Vertex)},
            };
            std::vector<uint32_t> indices;
            indices.resize(part.num_triangles * 3);
            size_t num_vertices = ufbx_generate_indices(
                streams, 1, indices.data(), indices.size(), nullptr, nullptr);
            vertices.resize(num_vertices);
            Mesh meshy;
            meshy._vertices = std::move(vertices);
            meshy._indices = indices;

            Meshes.push_back({meshy, indices});
        }
    }
}