#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"
#include "RenderProgram.h"
#include "RenderProgramManager.h"

static bool reloadRenderPrograms = false;

sdRenderProgramManager::sdRenderProgramManager(void)
{
}

sdRenderProgramManager::~sdRenderProgramManager(void)
{
    programs.DeleteContents(true);
}

const sdRenderProgram * sdRenderProgramManager::Alloc(const char *name) {
    sdRenderProgram *program = new sdRenderProgram;

    programs.Append(program);
    program->LoadProgram(name);

    return program;
}

const sdRenderProgram * sdRenderProgramManager::LoadProgram(const char *name) {
    const sdRenderProgram *program;

    program = Find(name);
    if (program)
        return program;

    program = Alloc(name);

    return program;
}

const sdRenderProgram * sdRenderProgramManager::Find(const char *name) {
    const idDecl* decl = declManager->FindType(DECL_RENDERPROGRAM, name, false);
    if (!decl) {
        common->Warning("sdRenderProgramManager::Find: render program decl '%s' not found", name);
        return NULL;
    }

    for (int i = 0; i < programs.Num(); i++) {
        if (programs[i]->GetDeclRenderProgram() == decl) {
            return programs[i];
        }
    }

    return NULL;
}

void sdRenderProgramManager::ReloadAll(void) {
	idStrList shaderNames;
	shaderNames.Resize(programs.Num());
    for (int i = 0; i < programs.Num(); i++) {
        shaderNames.Append(programs[i]->GetDeclRenderProgram()->GetName());
    }

	if(shaderNames.Num() > 0)
		shaderManager->ReloadShaders(shaderNames);
}

void sdRenderProgramManager::CheckCVars(void) {
#if 1
	const sdDeclRenderProgram *program;

	static idCVar * const ShaderCVars[] = {
		&r_shaderQuality,
		&r_megaDrawMethod,
		&r_normalizeNormalMaps,
		&r_dxnNormalMaps,
		&r_32ByteVtx,
		&r_useDitherMask,
		&r_shaderSkipSpecCubeMaps,
		&alphatest_kill,
		&r_detailTexture,
		&r_megaMultiply,
		&r_useARBPositionInvariant,
		&r_skipDiffuse,
		&r_skipBump,
	};
	static idStaticList<const idCVar *, sizeof(ShaderCVars) / sizeof(ShaderCVars[0])> changes;

	for (int i = 0; i < sizeof(ShaderCVars) / sizeof(ShaderCVars[0]); i++) {
		if(ShaderCVars[i]->IsModified())
		{
			changes.Append(ShaderCVars[i]);
			ShaderCVars[i]->ClearModified();
		}
	}

	if (changes.Num() == 0)
		return;

	idStrList shaderNames;
	shaderNames.Resize(programs.Num());
    for (int i = 0; i < programs.Num(); i++) {
    	program = programs[i]->GetDeclRenderProgram();
    	for (int m = 0; m < changes.Num(); m++)
    	{
    		if(program->HasDefine(changes[m]->GetName()))
    		{
    			shaderNames.Append(program->GetName());
    			break;
    		}
    	}
	}

	if(shaderNames.Num() > 0)
		shaderManager->ReloadShaders(shaderNames);

	changes.Clear();
#else
	bool changed = false;
	if(r_shaderQuality.IsModified())
	{
		changed = true;
		r_shaderQuality.ClearModified();
	}
	if(r_megaDrawMethod.IsModified())
	{
		changed = true;
		r_megaDrawMethod.ClearModified();
	}
	if(r_normalizeNormalMaps.IsModified())
	{
		changed = true;
		r_normalizeNormalMaps.ClearModified();
	}
	if(r_dxnNormalMaps.IsModified())
	{
		changed = true;
		r_dxnNormalMaps.ClearModified();
	}
	if(r_32ByteVtx.IsModified())
	{
		changed = true;
		r_32ByteVtx.ClearModified();
	}
	if(r_useDitherMask.IsModified())
	{
		changed = true;
		r_useDitherMask.ClearModified();
	}
	if(r_shaderSkipSpecCubeMaps.IsModified())
	{
		changed = true;
		r_shaderSkipSpecCubeMaps.ClearModified();
	}
	if(alphatest_kill.IsModified())
	{
		changed = true;
		alphatest_kill.ClearModified();
	}
	
	if(changed)
		ReloadAll();
#endif
}

static sdRenderProgramManager renderProgramManagerLocal;
sdRenderProgramManager *renderProgramManager = &renderProgramManagerLocal;

void sdRenderProgramManager::LoadRenderProgram_f(const idCmdArgs &args) {
    if (args.Argc() < 2) {
        common->Printf("Usage: %s <shader name>\n", args.Argv(0));
        return;
    }

    renderProgramManagerLocal.LoadProgram(args.Argv(1));
}

void sdRenderProgramManager::ReloadAllRenderPrograms_f(const idCmdArgs &) {
#ifdef _MULTITHREAD
	if(multithreadActive)
		reloadRenderPrograms = true;
	else
#endif
    renderProgramManagerLocal.ReloadAll();
}

void sdRenderProgramManager::ListRenderPrograms_f(const idCmdArgs &args) {
	common->Printf("----- %d shader programs -----\n", renderProgramManagerLocal.programs.Num());

    for (int i = 0; i < renderProgramManagerLocal.programs.Num(); i++) {
        const sdRenderProgram *program = renderProgramManagerLocal.programs[i];
        if (program->IsValid()) {
            const shaderProgram_t *shader = shaderManager->Get(program->GetShaderProgram());
            common->Printf("[%2d] %s: loaded: type=%d(%s), handle=%d, OpenGL handle=%d, in %s\n", i, program->GetDeclRenderProgram()->GetName(),
                shader ? shader->type : -1, shader ? shader->type >= SHADER_CUSTOM ? "custom" : "built-in" : "unload", program->GetShaderProgram(),
                shader ? shader->program : -1, program->GetDeclRenderProgram()->GetFileName()
                );
        } else {
            common->Printf("[%2d] %s: not load, in %s\n", i,
                program->GetDeclRenderProgram()->GetName(), program->GetDeclRenderProgram()->GetFileName()
                );
        }
    }
}

void R_CheckRenderProgramCVars(void)
{
	bool changed = false;
	if(r_shaderQuality.IsModified())
	{
		changed = true;
		r_shaderQuality.ClearModified();
	}
	if(r_megaDrawMethod.IsModified())
	{
		changed = true;
		r_megaDrawMethod.ClearModified();
	}
	if(r_normalizeNormalMaps.IsModified())
	{
		changed = true;
		r_normalizeNormalMaps.ClearModified();
	}
	if(r_dxnNormalMaps.IsModified())
	{
		changed = true;
		r_dxnNormalMaps.ClearModified();
	}
	if(r_32ByteVtx.IsModified())
	{
		changed = true;
		r_32ByteVtx.ClearModified();
	}
	if(r_useDitherMask.IsModified())
	{
		changed = true;
		r_useDitherMask.ClearModified();
	}
	if(r_shaderSkipSpecCubeMaps.IsModified())
	{
		changed = true;
		r_shaderSkipSpecCubeMaps.ClearModified();
	}
	if(alphatest_kill.IsModified())
	{
		changed = true;
		alphatest_kill.ClearModified();
	}
	
	if(!changed)
		return;
#ifdef _MULTITHREAD
	if(multithreadActive)
		reloadRenderPrograms = true;
	else
#endif
	renderProgramManagerLocal.ReloadAll();
}

#ifdef _MULTITHREAD
void RB_ReloadRenderPrograms(void)
{
	if(reloadRenderPrograms && multithreadActive)
	{
		renderProgramManagerLocal.ReloadAll();
		reloadRenderPrograms = false;
	}
}
#endif
