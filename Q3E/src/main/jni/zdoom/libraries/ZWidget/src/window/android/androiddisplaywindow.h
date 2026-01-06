#pragma once

#include <list>
#include <unordered_map>
#include <zwidget/window/window.h>

class SDL2DisplayWindow : public DisplayWindow
{
public:
	SDL2DisplayWindow(DisplayWindowHost* windowHost);
	~SDL2DisplayWindow();

	void SetWindowTitle(const std::string& text) override;
	void SetWindowFrame(const Rect& box) override;
	void SetClientFrame(const Rect& box) override;
	void Show() override;
	void ShowFullscreen() override;
	void ShowMaximized() override;
	void ShowMinimized() override;
	void ShowNormal() override;
	void Hide() override;
	void Activate() override;
	void ShowCursor(bool enable) override;
	void LockCursor() override;
	void UnlockCursor() override;
	void CaptureMouse() override;
	void ReleaseMouseCapture() override;
	void Update() override;
	bool GetKeyState(EInputKey key) override;
	void SetCursor(StandardCursor cursor) override;

	Rect GetWindowFrame() const override;
	Size GetClientSize() const override;
	int GetPixelWidth() const override;
	int GetPixelHeight() const override;
	double GetDpiScale() const override;

	void PresentBitmap(int width, int height, const uint32_t* pixels) override;

	void SetBorderColor(uint32_t bgra8) override;
	void SetCaptionColor(uint32_t bgra8) override;
	void SetCaptionTextColor(uint32_t bgra8) override;

	std::string GetClipboardText() override;
	void SetClipboardText(const std::string& text) override;

	static void DispatchEvent(const void *event);
	static SDL2DisplayWindow* FindEventWindow(const void *event);

	void OnWindowEvent(const void *event);
	void OnTextInput(const void *event);
	void OnKeyUp(const void *event);
	void OnKeyDown(const void *event);
	void OnMouseButtonUp(const void *event);
	void OnMouseButtonDown(const void *event);
	void OnMouseWheel(const void *event);
	void OnMouseMotion(const void *event);
	void OnPaintEvent();

	EInputKey GetMouseButtonKey(const void *event);

	static EInputKey ScancodeToInputKey(int keycode);
	static int InputKeyToScancode(EInputKey inputkey);

	template<typename T>
	Point GetMousePos(const T& event)
	{
		double uiscale = GetDpiScale();
		return Point(event.x / uiscale, event.y / uiscale);
	}

	static void ProcessEvents();
	static void RunLoop();
	static void ExitLoop();
	static Size GetScreenSize();

	static void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer);
	static void StopTimer(void* timerID);

	DisplayWindowHost* WindowHost = nullptr;
	void* WindowHandle = nullptr; // SDL_Window
	void* RendererHandle = nullptr; // SDL_Renderer
	void* BackBufferTexture = nullptr; // SDL_Texture
	int BackBufferWidth = 0;
	int BackBufferHeight = 0;

	static bool ExitRunLoop;
	static uint32_t PaintEventNumber;
	static std::unordered_map<int, SDL2DisplayWindow*> WindowList;
};
