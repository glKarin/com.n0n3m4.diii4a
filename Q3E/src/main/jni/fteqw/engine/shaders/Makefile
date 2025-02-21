
#NOTE: if you're on windows, you might want to use CC=i686-pc-mingw32-gcc

CC ?= gcc
VKSDKPATH ?= ~/VulkanSDK/1.1.73.0/x86_64/bin/

all:

NAMES=	fixedemu	fixedemu_flat altwater	bloom_blur	bloom_filter	bloom_final	colourtint	crepuscular_opaque	crepuscular_rays	crepuscular_sky	depthonly	default2d	defaultadditivesprite		defaultskin	defaultsky	defaultskybox	defaultfill	defaultsprite	defaultwall	defaultwarp	defaultgammacb	drawflat_wall	lpp_depthnorm	lpp_light	lpp_wall	postproc_fisheye	postproc_panorama	postproc_laea	postproc_stereographic	postproc_equirectangular   postproc_panini	fxaa	underwaterwarp	menutint	terrain	rtlight


ALLNAMES+=$(foreach v,$(NAMES),glsl/$v.glsl)
ALLNAMES+=$(foreach v,$(NAMES),hlsl9/$v.hlsl)
ALLNAMES+=$(foreach v,$(NAMES),hlsl11/$v.hlsl)
ALLNAMES:=$(realpath $(ALLNAMES))

VKNAMES=$(realpath $(foreach v,$(NAMES),vulkan/$v.glsl))
VKNAMES+=postproc_fisheye
VKNAMES+=rq_rtlight
VKNAMES:=$(foreach v,$(VKNAMES),vulkanblobs/$(notdir $(basename $v)).fvb)
ALLNAMES+=$(VKNAMES)

generatebuiltinsl: generatebuiltinsl.c
	$(CC) $< -o $@

makevulkanblob: makevulkanblob.c
	$(CC) $< -o $@

vulkanblobs/rq_%.fvb: vulkan/%.glsl makevulkanblob vulkan/sys/defs.h vulkan/sys/fog.h vulkan/sys/offsetmapping.h vulkan/sys/skeletal.h
	@echo Making $@ from $<
	@PATH=$(PATH):$(VKSDKPATH) ./makevulkanblob $< $@ rq
vulkanblobs/%.fvb: vulkan/%.glsl makevulkanblob vulkan/sys/defs.h vulkan/sys/fog.h vulkan/sys/offsetmapping.h vulkan/sys/skeletal.h
	@echo Making $@ from $<
	@PATH=$(PATH):$(VKSDKPATH) ./makevulkanblob $< $@

#vulkanblobs/%.fvb: glsl/%.glsl makevulkanblob vulkan/sys/defs.h vulkan/sys/fog.h vulkan/sys/offsetmapping.h vulkan/sys/skeletal.h
#	./makevulkanblob $< $@


all: generatebuiltinsl $(ALLNAMES)
	./generatebuiltinsl
