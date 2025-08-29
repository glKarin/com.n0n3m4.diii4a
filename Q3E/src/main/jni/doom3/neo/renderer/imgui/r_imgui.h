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
void RB_ImGui_Stop(void);
void RB_ImGui_Shutdown(void);

void R_ImGui_Ready(GLimp_ImGui_Render_f draw, GLimp_ImGui_Render_f begin, GLimp_ImGui_Render_f end, void *data, bool grabMouse);
void R_ImGui_HandleCallback(void);
bool R_ImGui_IsRunning(void);
bool R_ImGui_IsGrabMouse(void);

void ImGui_MouseWheelEvent(float wx, float wy);
void ImGui_HandleEvent(const sysEvent_t *ev);
void R_ImGui_idTech4AmmSettings_f(const idCmdArgs &args);

#endif
