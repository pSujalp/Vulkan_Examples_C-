#pragma once

#include "ufbx.h"
#include <stdio.h>
#include <unordered_map>
#include <iostream>
#include <map>
#include <set>

namespace FBX_ANIM
{

    class FBX_ANIMATION
    {
    public:
        ufbx_scene *scene;
        std::unordered_map<ufbx_node *, std::map<double, ufbx_transform>> nodes_transforms;
        std::set<double> serial_timerange; 


        
        FBX_ANIMATION() = default;

        FBX_ANIMATION(ufbx_scene *scene)
        {
            this->scene = scene;
            for (ufbx_anim_stack *stack : scene->anim_stacks)
            {
                printf("stack %s:\n", stack->name.data);
                bake_animation(scene, stack->anim);
            }
        }

        void bake_animation(ufbx_scene *scene, ufbx_anim *anim)
        {

            ufbx_baked_anim *bake = ufbx_bake_anim(scene, anim, NULL, NULL);
            if (!bake)
                return;
            for (const ufbx_baked_node &bake_node : bake->nodes)
            {
                std::map<double, ufbx_transform> key_to_TRS;
                ufbx_node *scene_node = scene->nodes[bake_node.typed_id];

                if (scene_node->mesh)
                {
                    ufbx_mesh *mesh = scene_node->mesh;
                    std::cout << "Mesh element id :"<<mesh->element_id<< std::endl;
                }

                for (const auto &i : bake_node.translation_keys)
                {
                    ufbx_transform transform = ufbx_evaluate_transform(anim, scene_node, i.time);
                    key_to_TRS[i.time] = transform;
                    serial_timerange.insert(i.time);
                }
                for (const auto &i : bake_node.rotation_keys)
                {
                    ufbx_transform transform = ufbx_evaluate_transform(anim, scene_node, i.time);
                    key_to_TRS[i.time] = transform;
                    serial_timerange.insert(i.time);
                }
                for (const auto &i : bake_node.scale_keys)
                {
                    ufbx_transform transform = ufbx_evaluate_transform(anim, scene_node, i.time);
                    key_to_TRS[i.time] = transform;
                    serial_timerange.insert(i.time);
                }
                nodes_transforms[scene_node] = std::move(key_to_TRS);
            }

            // for(const auto &i : key_to_TRS){

            //     std::cout << i.first << std::endl <<"  T:" << i.second.translation.x << "," << i.second.translation.y << "," << i.second.translation.z<<std::endl;

            //     std::cout << "  R:" << i.second.rotation.x << "," << i.second.rotation.y << "," << i.second.rotation.z<<std::endl;

            //     std::cout << "  S:" << i.second.scale.x << "," << i.second.scale.y << "," << i.second.scale.z<<std::endl;
            // }
            ufbx_free_baked_anim(bake);
        }
    };

}