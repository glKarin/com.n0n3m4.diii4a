// edit_guis.h
//

//
// rvmToolGui
//
class rvmToolGui {
public:
	virtual const char* Name(void) = 0;
	virtual void Render(void) = 0;
};

//
// rvmToolShowFrameRate
//
class rvmToolShowFrameRate : public rvmToolGui {
public:
	virtual const char* Name(void);
	virtual void Render(void);
};

extern rvmToolShowFrameRate displayFrameRateTool;