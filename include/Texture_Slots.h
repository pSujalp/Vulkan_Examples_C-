#pragma once

#include "vk_types.h"
#include "vk_initializers.h"
#include <unordered_map>

class Texture_Slots
{

public:
	VkDevice vkdevice;
	DeletionQueue dq;
	VkDescriptorPool _descriptorPool;
	std::vector<VkDescriptorSetLayout> vkVkDescriptorSet_array;
	std::unordered_map<VkDescriptorSet, uint8_t> TextureDescriptor_map;

	Texture_Slots(VkDevice _device, DeletionQueue deletionQ, VkDescriptorPool dp) : vkdevice(_device), dq(deletionQ), _descriptorPool(dp) {}

	Texture_Slots() {}

	void Add(const AllocatedImage &_texture, const int &setnumber, const int &bind = 0)
	{

		VkDescriptorSetLayout _TextureSetLayout = VK_NULL_HANDLE;
		;
		VkSampler _TextureSampler = VK_NULL_HANDLE;
		VkImageView _textureImageView = VK_NULL_HANDLE;
		VkDescriptorSet TextureDescriptor = VK_NULL_HANDLE;

		VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bind);

		VkDescriptorSetLayoutCreateInfo set3info = {};
		set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set3info.bindingCount = 1;
		set3info.flags = 0;
		set3info.pNext = nullptr;
		set3info.pBindings = &textureBind;

		vkCreateDescriptorSetLayout(vkdevice, &set3info, nullptr, &_TextureSetLayout);

		VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
		VK_CHECK(vkCreateSampler(vkdevice, &samplerInfo, nullptr, &_TextureSampler));

		dq.push_function([=]()
						 { vkDestroySampler(vkdevice, _TextureSampler, nullptr);
	   vkDestroyDescriptorSetLayout(vkdevice, _TextureSetLayout, nullptr); });

		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, _texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
		VK_CHECK(vkCreateImageView(vkdevice, &imageInfo, nullptr, &_textureImageView));

		dq.push_function([=]()
						 { vkDestroyImageView(vkdevice, _textureImageView, nullptr); });

		VkDescriptorSetAllocateInfo texAllocInfo = {};
		texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocInfo.pNext = nullptr;
		texAllocInfo.descriptorPool = _descriptorPool;
		texAllocInfo.descriptorSetCount = 1;
		texAllocInfo.pSetLayouts = &_TextureSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(vkdevice, &texAllocInfo, &TextureDescriptor));

		vkVkDescriptorSet_array.emplace_back(_TextureSetLayout);

		VkDescriptorImageInfo imageBufferInfo = {};
		imageBufferInfo.sampler = _TextureSampler;
		imageBufferInfo.imageView = _textureImageView;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet Writetexture0 = {};
		Writetexture0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		Writetexture0.pNext = nullptr;
		Writetexture0.dstBinding = bind;
		Writetexture0.dstSet = TextureDescriptor;
		Writetexture0.descriptorCount = 1;
		Writetexture0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Writetexture0.pImageInfo = &imageBufferInfo;

		TextureDescriptor_map[TextureDescriptor] = setnumber;

		vkUpdateDescriptorSets(vkdevice, 1, &Writetexture0, 0, nullptr);
	}

	void Add(VkImageView _textureImageView, VkSampler _TextureSampler, const int &setnumber, const int &bind = 0)
	{
		VkDescriptorSetLayout _TextureSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet TextureDescriptor = VK_NULL_HANDLE;

		VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bind);

		VkDescriptorSetLayoutCreateInfo set3info = {};
		set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set3info.bindingCount = 1;
		set3info.flags = 0;
		set3info.pNext = nullptr;
		set3info.pBindings = &textureBind;

		vkCreateDescriptorSetLayout(vkdevice, &set3info, nullptr, &_TextureSetLayout);

		dq.push_function([=]()
						 { vkDestroyDescriptorSetLayout(vkdevice, _TextureSetLayout, nullptr); });
		
		

		VkDescriptorSetAllocateInfo texAllocInfo = {};
		texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocInfo.pNext = nullptr;
		texAllocInfo.descriptorPool = _descriptorPool;
		texAllocInfo.descriptorSetCount = 1;
		texAllocInfo.pSetLayouts = &_TextureSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(vkdevice, &texAllocInfo, &TextureDescriptor));

		vkVkDescriptorSet_array.emplace_back(_TextureSetLayout);

		VkDescriptorImageInfo imageBufferInfo = {};
		imageBufferInfo.sampler = _TextureSampler;
		imageBufferInfo.imageView = _textureImageView;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet Writetexture0 = {};
		Writetexture0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		Writetexture0.pNext = nullptr;
		Writetexture0.dstBinding = bind;
		Writetexture0.dstSet = TextureDescriptor;
		Writetexture0.descriptorCount = 1;
		Writetexture0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Writetexture0.pImageInfo = &imageBufferInfo;

		TextureDescriptor_map[TextureDescriptor] = setnumber;

		vkUpdateDescriptorSets(vkdevice, 1, &Writetexture0, 0, nullptr);
	}
};