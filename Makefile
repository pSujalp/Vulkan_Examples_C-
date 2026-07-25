
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
SHADER_DIR := shaders

CMAKE := cmake
GLSLANG := glslangValidator

VERT_SHADERS := $(wildcard $(SHADER_DIR)/*.vert)
FRAG_SHADERS := $(wildcard $(SHADER_DIR)/*.frag)
COMP_SHADERS := $(wildcard $(SHADER_DIR)/*.comp)
GEOM_SHADERS := $(wildcard $(SHADER_DIR)/*.geom)
TESC_SHADERS := $(wildcard $(SHADER_DIR)/*.tesc)
TESE_SHADERS := $(wildcard $(SHADER_DIR)/*.tese)
MESH_SHADERS := $(wildcard $(SHADER_DIR)/*.mesh)
TASK_SHADERS := $(wildcard $(SHADER_DIR)/*.task)
RGEN_SHADERS := $(wildcard $(SHADER_DIR)/*.rgen)
RCHIT_SHADERS := $(wildcard $(SHADER_DIR)/*.rchit)
RMISS_SHADERS := $(wildcard $(SHADER_DIR)/*.rmiss)

SHADERS := \
	$(VERT_SHADERS) \
	$(FRAG_SHADERS) \
	$(COMP_SHADERS) \
	$(GEOM_SHADERS) \
	$(TESC_SHADERS) \
	$(TESE_SHADERS) \
	$(MESH_SHADERS) \
	$(TASK_SHADERS) \
	$(RGEN_SHADERS) \
	$(RCHIT_SHADERS) \
	$(RMISS_SHADERS)

SPV := $(SHADERS:%=%.spv)


all: shaders configure build copy


configure:
	$(CMAKE) -S . -B $(BUILD_DIR)


build:
	$(CMAKE) --build $(BUILD_DIR) -j


shaders: $(SPV)

%.vert.spv: %.vert
	$(GLSLANG) -V $< -o $@

%.frag.spv: %.frag
	$(GLSLANG) -V $< -o $@

%.comp.spv: %.comp
	$(GLSLANG) -V $< -o $@

%.geom.spv: %.geom
	$(GLSLANG) -V $< -o $@

%.tesc.spv: %.tesc
	$(GLSLANG) -V $< -o $@

%.tese.spv: %.tese
	$(GLSLANG) -V $< -o $@

%.mesh.spv: %.mesh
	$(GLSLANG) -V $< -o $@

%.task.spv: %.task
	$(GLSLANG) -V $< -o $@

%.rgen.spv: %.rgen
	$(GLSLANG) -V $< -o $@

%.rchit.spv: %.rchit
	$(GLSLANG) -V $< -o $@

%.rmiss.spv: %.rmiss
	$(GLSLANG) -V $< -o $@


copy:
	mkdir -p $(BIN_DIR)
	cp -R assets $(BIN_DIR)/
	cp -R shaders $(BIN_DIR)/


run: all
	$(BIN_DIR)/Vulkan_tutorial

rebuild:
	rm -rf $(BUILD_DIR)
	$(MAKE) all

clean:
	rm -rf $(BUILD_DIR)
	find shaders -name "*.spv" -delete

.PHONY: all configure build shaders copy run rebuild clean