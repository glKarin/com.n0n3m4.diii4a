#include "../../idlib/precompiled.h"

#include "imgui.h"

enum {
    IG_CALLBACK_EXIT = 1,
    IG_CALLBACK_UPDATE_CVAR,
    IG_CALLBACK_COMMAND,
};

#define IG_USER_EVENT_DATA_LENGTH 64
#define IG_COMMAND_NAME "imgui_command"

#if 1
#if !defined(_MSC_VER)
#define IMGUI_DEBUG(fmt, args...) printf(fmt, ##args);
#else
#define IMGUI_DEBUG(fmt, ...) printf(fmt, __VA_ARGS__);
#endif
#else
#define IMGUI_DEBUG(fmt, ...)
#endif

enum {
    IG_SE_USER = SE_CONSOLE + 1,
};

enum {
    IG_USER_EVENT_RESET = 1,
    IG_USER_EVENT_EXIT,
};

typedef struct imGuiUserEvent_s {
    int type;
    char str[IG_USER_EVENT_DATA_LENGTH];
} imGuiUserEvent_t;

typedef struct imGuiEvent_s
{
    int type;
    union {
        struct {
            ImGuiKey key;
            int state;
        } key;
        struct {
            int x;
            int y;
        } mouse;
        struct {
            int ch;
        } input;
        struct {
            float dx;
            float dy;
        } wheel;
        imGuiUserEvent_t user;
    } data;
} imGuiEvent_t;

typedef struct imGuiCallback_s
{
    int type;
    idStr key;
    idStr value;
} imGuiCallback_t;

class idImGui
{
public:
    idImGui(void);

    void Init(void); // backend
    void Render(void); // backend
    void Shutdown(void); // backend
    void Start(void); // backend
    void Stop(void); // backend
    bool IsInitialized(void) const {
        return isInitialized;
    }
    bool IsRunning(void) const {
        return running;
    }
    bool IsReady(void) const {
        return ready;
    }
    bool IsEventRunning(void) const {
        return eventRunning;
    }
    bool HasRenderer(void) const {
        return draw != NULL;
    }
    bool IsGrabMouse(void) const {
        return grabMouse;
    }
    void RegisterRenderer(GLimp_ImGui_Render_f func, GLimp_ImGui_Render_f begin, GLimp_ImGui_Render_f end, void *data = NULL); // frontend
    void Ready(bool grabMouse); // frontend
    void StartEvent(void); // frontend
    void ExitEvent(void); // backend
    void PushEvent(const imGuiEvent_t &ev) { // frontend write
        IMGUI_DEBUG("Frontend::Push event -> %d\n", ev.type);
        events.Append(ev);
    }
    void HandleEvent(const imGuiEvent_t &ev); // backend read
    void PullEvent(void); // backend read
    void PushCallback(const imGuiCallback_t &cb) { // backend write
        IMGUI_DEBUG("Backend::Push callback -> %d\n", cb.type);
        callbacks.Append(cb);
    }
    void HandleCallback(const imGuiCallback_t &cb); // frontend read
    void PullCallback(void); // frontend read

protected:
    enum {
        IG_FLAG_RESET_POSITION = BIT(0),
        IG_FLAG_RESET_SIZE = BIT(1),
    };
    void NewFrame(void); // backend
    void EndFrame(void); // backend
    static void Demo(void *);
    void HandleUserEvent(const imGuiUserEvent_t &ev); // backend read
    bool CheckFlag(int what, bool reset);

private:
    bool isInitialized; // backend write
    bool running; // backend write
    GLimp_ImGui_Render_f begin; // backend read; frontend write
    GLimp_ImGui_Render_f draw; // backend read; frontend write
    GLimp_ImGui_Render_f end; // backend read; frontend write
    void *data; // backend read; frontend write
    bool grabMouse; // frontend write
    bool ready; // frontend write
    bool eventRunning; // frontend write
    idList<imGuiEvent_t> events; // backend read; frontend write
    idList<imGuiCallback_t> callbacks; // frontend read; backend write
    int flags; // internal flags
};

idImGui::idImGui(void)
: isInitialized(false),
  running(false),
  begin(NULL),
  draw(NULL),
  end(NULL),
  data(NULL),
  grabMouse(false),
  ready(false),
  eventRunning(false),
  flags(0)
{
    draw = &idImGui::Demo;
    events.SetGranularity(1);
    callbacks.SetGranularity(1);
}

void idImGui::Init(void)
{
    if(isInitialized)
        return;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    GLimp_ImGui_Init();

    if(imgui_fontScale.GetFloat() > 0.0f)
    {
        ImFontConfig font_cfg;
        font_cfg.SizePixels = imgui_fontScale.GetFloat();
        io.Fonts->AddFontDefault(&font_cfg);
    }

    // Arbitrary scale-up
    // FIXME: Put some effort into DPI awareness
    ImGui::GetStyle().ScaleAllSizes(imgui_scale.GetFloat() > 0.0f ? imgui_scale.GetFloat() : 1.0f);

    running = false;
    isInitialized = true;

    IMGUI_DEBUG("Backend::Init\n");
}

void idImGui::NewFrame(void)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::GetStyle().ScaleAllSizes(1.0f);
/*    if(imgui_scale.IsModified())
    {
        ImGui::GetStyle().ScaleAllSizes(1.0f);
        imgui_scale.ClearModified();
    }

    if(imgui_fontScale.IsModified())
    {
        float fontScale = imgui_fontScale.GetFloat() > 0.0f ? imgui_fontScale.GetFloat() : 22.0f;
        printf("FFF %f\n", fontScale);
        io.Fonts->ConfigData[0].SizePixels = fontScale;
        ImGui::SetWindowFontScale(fontScale);
        imgui_fontScale.ClearModified();
    }*/

    // Start the Dear ImGui frame
    GLimp_ImGui_NewFrame();
    ImGui::NewFrame();
}

void idImGui::EndFrame(void)
{
    GLimp_ImGui_EndFrame();
}

void idImGui::Render(void)
{
    if(!running)
        return;

    if(begin)
        begin(data);
    NewFrame();
    {
        if(CheckFlag(IG_FLAG_RESET_POSITION, true))
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        else
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Appearing);
        if(CheckFlag(IG_FLAG_RESET_SIZE, true))
            ImGui::SetNextWindowSize(ImVec2(glConfig.vidWidth, glConfig.vidHeight), ImGuiCond_Always);
        else
            ImGui::SetNextWindowSize(ImVec2(glConfig.vidWidth, glConfig.vidHeight), ImGuiCond_Appearing);

        if(draw)
            draw(data);

        // Rendering
        ImGui::Render();

        if(ImGui::IsKeyReleased(ImGuiKey_Escape))
        {
            RB_ImGui_Stop();
        }
    }
    EndFrame();

    if(end)
        end(data);
}

void idImGui::Shutdown(void)
{
    if(!isInitialized)
        return;
    // Cleanup
    GLimp_ImGui_Shutdown();
    ImGui::DestroyContext();
    running = false;
    isInitialized = false;

    IMGUI_DEBUG("Backend::Shutdown\n");
}

void idImGui::RegisterRenderer(GLimp_ImGui_Render_f d, GLimp_ImGui_Render_f b, GLimp_ImGui_Render_f e, void *p)
{
    if(eventRunning)
        return;
    draw = d;
    if(draw)
    {
        begin = b;
        end = e;
        data = p;
    }
    else
    {
        begin = NULL;
        end = NULL;
        data = NULL;
    }

    IMGUI_DEBUG("Frontend::Register renderer\n");
}

void idImGui::StartEvent(void)
{
    if(running)
        return;
    if(eventRunning)
        return;
    callbacks.Clear();
    eventRunning = true;
    IMGUI_DEBUG("Frontend::Start event\n");
}

void idImGui::Start(void)
{
    if(!isInitialized)
        return;
    if(running)
        return;
#if 1
    events.Clear();
#endif
    running = true;
    IMGUI_DEBUG("Backend::Start\n");
}

void idImGui::Stop(void)
{
    if(!running)
        return;
    running = false;
#if 1
    events.Clear();
#endif
    IMGUI_DEBUG("Backend::Stop\n");
}

void idImGui::Ready(bool m)
{
    if(running)
        return;
    if(ready)
        return;
    grabMouse = m;
    ready = true;
    IMGUI_DEBUG("Frontend::Ready\n");
}

void idImGui::ExitEvent(void)
{
    if(!eventRunning)
        return;
    eventRunning = false;
    ready = false;
    grabMouse = false;
    IMGUI_DEBUG("Frontend::Exit event\n");
}

void idImGui::HandleEvent(const imGuiEvent_t &ev)
{
    IMGUI_DEBUG("Backend::Handle event -> %d\n", ev.type);
    ImGuiIO& io = ImGui::GetIO();
    switch (ev.type) {
        case SE_MOUSE:
            io.AddMousePosEvent((float)ev.data.mouse.x, (float)ev.data.mouse.y);
            return;
        case SE_JOYSTICK_AXIS:
            io.AddMouseWheelEvent(ev.data.wheel.dx, ev.data.wheel.dy);
            return;
        case SE_KEY:
            if(ev.data.key.key >= ImGuiKey_MouseLeft && ev.data.key.key <= ImGuiKey_MouseX2)
                io.AddMouseButtonEvent(ev.data.key.key - ImGuiKey_MouseLeft, ev.data.key.state);
            else
                io.AddKeyEvent(ev.data.key.key, ev.data.key.state);
            return;
        case SE_CHAR:
            io.AddInputCharacter(ev.data.input.ch);
            return;
        case IG_SE_USER:
            HandleUserEvent(ev.data.user);
            return;
        default:
            return;
    }
}

bool idImGui::CheckFlag(int what, bool reset)
{
    int res = flags & what;
    if(reset)
        flags &= ~res;
    return res ? true : false;
}

void idImGui::HandleUserEvent(const imGuiUserEvent_t &ev)
{
    switch (ev.type) {
        case IG_USER_EVENT_RESET: {
            if(!idStr::Icmp("position", ev.str))
            {
                flags |= IG_FLAG_RESET_POSITION;
            }
            else if(!idStr::Icmp("size", ev.str))
            {
                flags |= IG_FLAG_RESET_SIZE;
            }
        }
            break;
        case IG_USER_EVENT_EXIT:
            ExitEvent();
            break;
        default:
            return;
    }
}

void idImGui::PullEvent(void)
{
    if(!events.Num())
        return;
    for(int i = 0; i < events.Num(); i++)
    {
        HandleEvent(events[i]);
    }
    events.Clear();
}

void idImGui::HandleCallback(const imGuiCallback_t &cb)
{
    IMGUI_DEBUG("Frontend::Handle callback -> %d\n", cb.type);
    switch (cb.type) {
        case IG_CALLBACK_UPDATE_CVAR:
            cvarSystem->SetCVarString(cb.key, cb.value);
            return;
        case IG_CALLBACK_COMMAND:
			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, cb.key);
            return;
        case IG_CALLBACK_EXIT:
            ExitEvent();
            return;
    }
}

void idImGui::PullCallback(void)
{
    if(!callbacks.Num())
        return;
    for(int i = 0; i < callbacks.Num(); i++)
    {
        HandleCallback(callbacks[i]);
    }
    callbacks.Clear();
}

void idImGui::Demo(void *)
{
    ImGuiIO& io = ImGui::GetIO();
    // Our state
    // (we use static, which essentially makes the variable globals, as a convenience to keep the example code easy to follow)
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

static idImGui imGuiBackend;

// backend
void RB_ImGui_Render(void)
{
    if(!imGuiBackend.IsRunning())
        return;
    imGuiBackend.Render();
}

// backend
bool RB_ImGui_IsRunning(void)
{
    return imGuiBackend.IsRunning();
}

// backend
void RB_ImGui_Start(void)
{
    if(imGuiBackend.IsRunning())
    {
        imGuiBackend.PullEvent();
        if(!imGuiBackend.IsEventRunning())
            RB_ImGui_Stop();
        return;
    }
    if(!imGuiBackend.IsEventRunning())
        return;
    if(!imGuiBackend.IsInitialized())
        imGuiBackend.Init();
    imgui_scale.SetModified();
    imgui_fontScale.SetModified();
    imGuiBackend.Start();
}

// backend
void RB_ImGui_PushExitCallback(void)
{
    imGuiCallback_t cb;
    cb.type = IG_CALLBACK_EXIT;
    imGuiBackend.PushCallback(cb);
}

// backend
void RB_ImGui_Stop(void)
{
    if(!imGuiBackend.IsRunning())
        return;
    imGuiBackend.Stop();
#ifdef _MULTITHREAD
	if(multithreadActive)
		RB_ImGui_PushExitCallback();
	else
#endif
	imGuiBackend.ExitEvent();
}

// backend
void RB_ImGui_Shutdown(void)
{
    imGuiBackend.Shutdown();
}

// frontend
void R_ImGui_SetRenderer(GLimp_ImGui_Render_f draw, GLimp_ImGui_Render_f begin, GLimp_ImGui_Render_f end, void *data)
{
    if(imGuiBackend.IsRunning())
        return;
    imGuiBackend.RegisterRenderer(draw, begin, end, data);
}

// frontend
void R_ImGui_Ready(bool grabMouse)
{
    if(!imGuiBackend.HasRenderer())
        return;
    if(imGuiBackend.IsEventRunning())
        return;
    imGuiBackend.Ready(grabMouse);

#ifdef _MULTITHREAD
	if(!multithreadActive)
#endif
	{
		R_ImGui_Render();
		RB_ImGui_Start();
	}
}

// frontend
bool R_ImGui_IsRunning(void)
{
    return imGuiBackend.IsEventRunning();
}

// frontend
void R_ImGui_Render(void)
{
    if(imGuiBackend.IsReady() && !imGuiBackend.IsEventRunning())
    {
        imGuiBackend.StartEvent();
    }
    else if(imGuiBackend.IsEventRunning())
    {
        imGuiBackend.PullCallback();
    }
}

// frontend
void R_ImGui_Stop(void)
{
    imGuiBackend.ExitEvent();
#ifdef _MULTITHREAD
    if(!multithreadActive)
#endif
        RB_ImGui_Stop();
}

// frontend
bool R_ImGui_IsGrabMouse(void)
{
    return imGuiBackend.IsGrabMouse();
}

// frontend
void R_ImGui_PushKeyEvent(ImGuiKey key, bool state)
{
    imGuiEvent_t ev;
    ev.type = SE_KEY;
    ev.data.key.key = key;
    ev.data.key.state = state;
    imGuiBackend.PushEvent(ev);
}

// frontend
void R_ImGui_PushMouseEvent(int x, int y)
{
    imGuiEvent_t ev;
    ev.type = SE_MOUSE;
    ev.data.mouse.x = x;
    ev.data.mouse.y = y;
    imGuiBackend.PushEvent(ev);
}

// frontend
void R_ImGui_PushWheelEvent(float dx, float dy)
{
    imGuiEvent_t ev;
    ev.type = SE_JOYSTICK_AXIS;
    ev.data.wheel.dx = dx;
    ev.data.wheel.dy = dy;
    imGuiBackend.PushEvent(ev);
}

// frontend
void R_ImGui_PushInputEvent(char ch)
{
    imGuiEvent_t ev;
    ev.type = SE_CHAR;
    ev.data.input.ch = ch;
    imGuiBackend.PushEvent(ev);
}

// frontend
void R_ImGui_PushImGuiEvent(int subType, const char data[IG_USER_EVENT_DATA_LENGTH])
{
    imGuiEvent_t ev;
    ev.type = IG_SE_USER;
    ev.data.user.type = subType;
    if(data)
        idStr::snPrintf(ev.data.user.str, sizeof(ev.data.user.str), "%s", data);
    else
        memset(ev.data.user.str, 0, sizeof(ev.data.user.str));
    imGuiBackend.PushEvent(ev);
}

// backend
void RB_ImGui_PushCVarCallback(const char *name, const char *value)
{
    imGuiCallback_t cb;
    cb.type = IG_CALLBACK_UPDATE_CVAR;
    cb.key = name;
    cb.value = value;
    imGuiBackend.PushCallback(cb);
}

void RB_ImGui_PushCVarCallback(const char *name, int value)
{
    char str[1024];
    idStr::snPrintf(str, sizeof(str), "%d", value);
    RB_ImGui_PushCVarCallback(name, str);
}

void RB_ImGui_PushCVarCallback(const char *name, float value)
{
    char str[1024];
    idStr::snPrintf(str, sizeof(str), "%f", value);
    RB_ImGui_PushCVarCallback(name, str);
}

void RB_ImGui_PushCVarCallback(const char *name, bool value)
{
    RB_ImGui_PushCVarCallback(name, (int)value);
}

// backend
void RB_ImGui_PushCmdCallback(const char *cmd)
{
    imGuiCallback_t cb;
    cb.type = IG_CALLBACK_COMMAND;
    cb.key = cmd;
    imGuiBackend.PushCallback(cb);
}

void R_ImGui_Command_f(const idCmdArgs &args)
{
#if 0
    if(!R_ImGui_IsRunning())
    {
        common->Warning("ImGui not running");
        return;
    }
#endif
    if(args.Argc() < 2)
    {
        common->Printf("Usage: %s reset|quit|exit|close [reset=position|size]\n", args.Argv(0));
        return;
    }
    const char *cmd = args.Argv(1);
    if(!idStr::Icmp("reset", cmd))
    {
        R_ImGui_PushImGuiEvent(IG_USER_EVENT_RESET, args.Argv(2));
    }
    else if(!idStr::Icmp("quit", cmd) || !idStr::Icmp("exit", cmd) || !idStr::Icmp("close", cmd))
    {
        R_ImGui_PushImGuiEvent(IG_USER_EVENT_EXIT, NULL);
    }
    else
    {
        common->Warning("Unknown ImGui command: %s", cmd);
        return;
    }

#if 1
    if(!R_ImGui_IsRunning())
    {
        imGuiBackend.PullEvent();
    }
#endif
}
