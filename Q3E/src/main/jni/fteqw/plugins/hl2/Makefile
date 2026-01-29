all: mat_vmt_progs.h

VMTPROGSBASE=lightmapped refract transition unlit vertexlit water rt
VMTPROGS:=$(foreach p,$(VMTPROGSBASE),vmt/$p)
VMTPROGSFILES:=$(foreach p,$(VMTPROGS),glsl/$p.glsl)
mat_vmt_progs.h: $(VMTPROGSFILES)
	../../engine/shaders/generatebuiltinsl $@ $(VMTPROGS)

