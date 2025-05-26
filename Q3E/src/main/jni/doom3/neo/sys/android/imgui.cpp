#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"

extern void ImGui_ImplAndroid_SetGeometry(int w, int h);

void GLimp_ImGui_Init(void)
{
    // Setup Platform/Renderer backends
#if 0
    ImGui_ImplAndroid_Init((ANativeWindow *)win);
#else
	ImGui_ImplAndroid_SetGeometry(screen_width, screen_height);
#endif
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
    ImGui_ImplAndroid_NewFrame();
}

void GLimp_ImGui_EndFrame(void)
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLimp_ImGui_Shutdown(void)
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
}


