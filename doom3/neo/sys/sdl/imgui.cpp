#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

void GLimp_ImGui_Init(void)
{
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
#ifdef _OPENGLES3
	if(USING_GLES3)
		ImGui_ImplOpenGL3_Init("#version 300 es");
	else
#endif
	ImGui_ImplOpenGL3_Init("#version 100");

    ImGuiIO& io = ImGui::GetIO();

	io.MouseDrawCursor = true;
}

void GLimp_ImGui_NewFrame(void)
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void GLimp_ImGui_EndFrame(void)
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLimp_ImGui_Shutdown(void)
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}


