#ifndef _KARIN_IMGUI_H
#define _KARIN_IMGUI_H

typedef void (* GLimp_ImGui_Render_f)(void *);
extern idCVar imgui_scale;
extern idCVar imgui_fontScale;

// in sys/XXXXX
void GLimp_ImGui_Init(void);
void GLimp_ImGui_Shutdown(void);
void GLimp_ImGui_NewFrame(void);
void GLimp_ImGui_EndFrame(void);

void RB_ImGui_Render(void);
bool RB_ImGui_IsRunning(void);
void RB_ImGui_Start(void);
void RB_ImGui_Stop(void);
void RB_ImGui_Shutdown(void);

void R_ImGui_SetRenderer(GLimp_ImGui_Render_f draw, GLimp_ImGui_Render_f begin = NULL, GLimp_ImGui_Render_f end = NULL, void *data = NULL);
void R_ImGui_Render(void);
bool R_ImGui_IsRunning(void);
void R_ImGui_Ready(bool grabMouse = false);
void R_ImGui_Stop(void);
bool R_ImGui_IsGrabMouse(void);

void ImGui_MouseWheelEvent(float wx, float wy);
void ImGui_HandleEvent(const sysEvent_t *ev);
void R_ImGui_idTech4AmmSettings_f(const idCmdArgs &args);

#endif
