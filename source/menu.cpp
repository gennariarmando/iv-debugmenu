#include "plugin.h"

#include "CTxdStore.h"
#include "CPad.h"
#include "CTimer.h"
#include "CPlayerInfo.h"
#include "CCamera.h"
#include "T_CB_Generic.h"

#include "menu.h"

#define snprintf _snprintf

#define strdup _strdup

#define MUHKEYS \
	X(leftjustdown, eKeyCodes::KEY_LEFT) \
	X(rightjustdown, eKeyCodes::KEY_RIGHT) \
	X(upjustdown, eKeyCodes::KEY_UP) \
	X(downjustdown, eKeyCodes::KEY_DOWN) \
	X(pgupjustdown, eKeyCodes::KEY_PRIOR) \
	X(pgdnjustdown, eKeyCodes::KEY_NEXT)

#define MUHBUTTONS \
	X(button1justdown, 1) \
	X(button2justdown, 2) \
	X(button3justdown, 3)

#define REPEATDELAY 700
#define REPEATINTERVAL 50
#define X(var, keycode) static int var;
MUHKEYS
#undef X
static int downtime;
static int repeattime;
static int lastkeydown;
static int* keyptr;

static int buttondown[3];
static int lastbuttondown;
static int* buttonptr;
static int button1justdown, button2justdown, button3justdown;
static float mouseX, mouseY;

static int menuOn = 0;
static int menuInitialized = 0;

static int firstBorder = 10;
static int topBorder = 10;
static int leading = 4;
static int gap = 10;
static int minwidth = 100;

static int fontscale = 1;
static int fontGlyphWidth = 8;
static int fontGlyphHeight = 16;
static int fontNumGlyphs = 256;
static int fontShadow = 0;
static int fontOutline = 1;

static CSprite2d cursorSprite, fontSprite, arrowSprite;

void drawMouse();
void drawArrow(rage::Vector4 r, int direction, int style);

Menu toplevel;
Menu* activeMenu = &toplevel;
Menu* deepestMenu = &toplevel;
Menu* mouseOverMenu;
MenuEntry* mouseOverEntry;
MenuEntry scrollUpEntry("SCROLLUP"), scrollDownEntry("SCROLLDOWN");	// dummies

DWORD prevMin, prevMag;

void RestoreRenderStates() {
    rage::GetD3DDevice()->SetSamplerState(0, D3DSAMP_MINFILTER, prevMin);
    rage::GetD3DDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, prevMag);
}

void SetRenderStates() {
    rage::GetD3DDevice()->GetSamplerState(0, D3DSAMP_MINFILTER, &prevMin);
    rage::GetD3DDevice()->GetSamplerState(0, D3DSAMP_MAGFILTER, &prevMag);
    rage::GetD3DDevice()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    rage::GetD3DDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
}

bool isMouseInRect(rage::Vector4 r) {
    return (mouseX >= r.left && mouseX < r.left + r.right &&
        mouseY >= r.top && mouseY < r.top + r.bottom);
}

/*
 * MenuEntry
 */

MenuEntry::MenuEntry(const char* name) {
    this->type = MENUEMPTY;
    this->name = strdup(name);
    this->next = NULL;
    this->menu = NULL;
}

MenuEntry_Sub::MenuEntry_Sub(const char* name, Menu* menu)
    : MenuEntry(name) {
    this->type = MENUSUB;
    this->submenu = menu;
}

MenuEntry_Var::MenuEntry_Var(const char* name, int vartype)
    : MenuEntry(name) {
    this->type = MENUVAR;
    this->vartype = vartype;
    this->maxvallen = 0;
    this->wrapAround = false;
}

/*
 * *****************************
 * MenuEntry_Int
 * *****************************
 */

MenuEntry_Int::MenuEntry_Int(const char* name)
    : MenuEntry_Var(name, MENUVAR_INT) {
}

#define X(NAME, TYPE, MAXLEN, FMT) \
int	MenuEntry_##NAME::findStringLen() { \
	TYPE i;							     \
	int len, maxlen = 0;					     \
	for(i = this->lowerBound; i <= this->upperBound; i++) { \
		len = strlen(this->strings[i-this->lowerBound]); \
		if(len > maxlen) \
			maxlen = len; \
	} \
	return maxlen; \
} \
void MenuEntry_##NAME::processInput(bool mouseOver, bool selected) { \
	TYPE v, oldv; \
	int overflow = 0; \
	int underflow = 0; \
 \
	v = *this->variable; \
	oldv = v; \
 \
	if((selected && leftjustdown) || (mouseOver && button3justdown)) { \
		v -= this->step; \
		if(v > oldv) \
			underflow = 1; \
	} \
	if((selected && rightjustdown) || (mouseOver && button1justdown)) { \
		v += this->step; \
		if(v < oldv) \
			overflow = 1; \
	} \
	if(this->wrapAround) { \
		if(v > this->upperBound || overflow) v = this->lowerBound; \
		if(v < this->lowerBound || underflow) v = this->upperBound; \
	} else { \
		if(v > this->upperBound || overflow) v = this->upperBound; \
		if(v < this->lowerBound || underflow) v = this->lowerBound; \
	} \
 \
	*this->variable = v; \
	if(oldv != v && this->triggerFunc) \
		this->triggerFunc(); \
} \
void MenuEntry_##NAME::getValStr(char *str, int len) { \
	static char tmp[20]; \
	if(this->strings) { \
		snprintf(tmp, 20, "%%%ds", this->maxvallen); \
		if(*this->variable < this->lowerBound || *this->variable > this->upperBound) { \
			snprintf(str, len, "ERROR"); \
			return; \
		} \
		snprintf(str, len, tmp, this->strings[*this->variable-this->lowerBound]); \
	} else \
		snprintf(str, len, this->fmt, *this->variable); \
} \
void MenuEntry_##NAME::setStrings(const char **strings) { \
	this->strings = strings; \
	if(this->strings) \
		this->maxvallen = findStringLen(); \
} \
MenuEntry_##NAME::MenuEntry_##NAME(const char *name, TYPE *ptr, TriggerFunc triggerFunc, TYPE step, TYPE lowerBound, TYPE upperBound, const char **strings) : MenuEntry_Int(name) { \
	this->variable = ptr; \
	this->step = step; \
	this->lowerBound = lowerBound; \
	this->upperBound = upperBound; \
	this->triggerFunc = triggerFunc; \
	this->maxvallen = MAXLEN; \
	this->fmt = FMT; \
	this->setStrings(strings); \
}
MUHINTS
#undef X

/*
 * *****************************
 * MenuEntry_Float
 * *****************************
 */

#define X(NAME, TYPE, MAXLEN, FMT) \
MenuEntry_##NAME::MenuEntry_##NAME(const char *name, TYPE *ptr, TriggerFunc triggerFunc, TYPE step, TYPE lowerBound, TYPE upperBound) : MenuEntry_Var(name, MENUVAR_FLOAT) { \
	this->variable = ptr; \
	this->step = step; \
	this->lowerBound = lowerBound; \
	this->upperBound = upperBound; \
	this->triggerFunc = triggerFunc; \
	this->maxvallen = MAXLEN; \
	this->fmt = FMT; \
} \
void MenuEntry_##NAME::getValStr(char *str, int len) { \
	snprintf(str, len, this->fmt, *this->variable); \
} \
void MenuEntry_##NAME::processInput(bool mouseOver, bool selected) { \
	float v, oldv; \
	int overflow = 0; \
	int underflow = 0; \
 \
	v = *this->variable; \
	oldv = v; \
 \
	if((selected && leftjustdown) || (mouseOver && button3justdown)) { \
		v -= this->step; \
		if(v > oldv) \
			underflow = 1; \
	} \
	if((selected && rightjustdown) || (mouseOver && button1justdown)){ \
		v += this->step; \
		if(v < oldv) \
			overflow = 1; \
	} \
	if(this->wrapAround){ \
		if(v > this->upperBound || overflow) v = this->lowerBound; \
		if(v < this->lowerBound || underflow) v = this->upperBound; \
	}else{ \
		if(v > this->upperBound || overflow) v = this->upperBound; \
		if(v < this->lowerBound || underflow) v = this->lowerBound; \
	} \
 \
	*this->variable = v; \
	if(oldv != v && this->triggerFunc) \
		this->triggerFunc(); \
}

    MUHFLOATS
#undef X

    /*
     * *****************************
     * MenuEntry_Cmd
     * *****************************
     */

void MenuEntry_Cmd::processInput(bool mouseOver, bool selected) {
    // Don't execute on button3
    if (this->triggerFunc && (selected && (leftjustdown || rightjustdown) || (mouseOver && button1justdown)))
        this->triggerFunc();
}

void MenuEntry_Cmd::getValStr(char* str, int len) {
    strncpy(str, "<", len);
}

MenuEntry_Cmd::MenuEntry_Cmd(const char* name, TriggerFunc triggerFunc)
    : MenuEntry_Var(name, MENUVAR_CMD) {
    this->maxvallen = 1;
    this->triggerFunc = triggerFunc;
}


/*
 * *****************************
 * Menu
 * *****************************
 */

void Menu::scroll(int off) {
    if (isScrollingUp && off < 0)
        scrollStart += off;
    if (isScrollingDown && off > 0)
        scrollStart += off;
    if (scrollStart < 0) scrollStart = 0;
    if (scrollStart > numEntries - numVisible) scrollStart = numEntries - numVisible;
}

void Menu::changeSelection(int newsel) {
    selection = newsel;
    if (selection < 0) selection = 0;
    if (selection >= numEntries) selection = numEntries - 1;
    if (selection < scrollStart) scrollStart = selection;
    if (selection >= scrollStart + numVisible) scrollStart = selection - numVisible + 1;
}

void Menu::changeSelection(MenuEntry* sel) {
    MenuEntry* e;
    int i = 0;
    for (e = this->entries; e; e = e->next) {
        if (e == sel) {
            this->selection = i;
            this->selectedEntry = sel;
            break;
        }
        i++;
    }
}

MenuEntry* Menu::findEntry(const char* entryname) {
    MenuEntry* m;
    for (m = this->entries; m; m = m->next)
        if (strcmp(entryname, m->name) == 0)
            return m;
    return NULL;
}

void Menu::insertEntrySorted(MenuEntry* entry) {
    MenuEntry** mp;
    int cmp;
    for (mp = &this->entries; *mp; mp = &(*mp)->next) {
        cmp = strcmp(entry->name, (*mp)->name);
        if (cmp == 0)
            return;
        if (cmp < 0)
            break;
    }
    entry->next = *mp;
    *mp = entry;
    entry->menu = this;
    this->numEntries++;
}

void Menu::appendEntry(MenuEntry* entry) {
    MenuEntry** mp;
    for (mp = &this->entries; *mp; mp = &(*mp)->next);
    entry->next = *mp;
    *mp = entry;
    entry->menu = this;
    this->numEntries++;
}

void Menu::update() {
    int i;
    int x, y;
    Pt sz;
    MenuEntry* e;
    int onscreen;
    x = this->r.left;
    y = this->r.top + 18;
    int end = this->r.top + this->r.bottom - 18;
    this->numVisible = 0;

    deepestMenu = this;

    int bottomy = end;
    onscreen = 1;
    i = 0;
    this->maxNameWidth = 0;
    this->maxValWidth = 0;
    this->isScrollingUp = this->scrollStart > 0;
    this->isScrollingDown = false;
    this->selectedEntry = NULL;
    for (e = this->entries; e; e = e->next) {
        sz = fontGetStringSize(e->name);
        e->r.left = x;
        e->r.top = y;
        e->r.right = sz.x;
        e->r.bottom = sz.y;

        if (i == this->selection)
            this->selectedEntry = e;

        if (i >= this->scrollStart)
            y += sz.y + leading * fontscale;
        if (y >= end) {
            this->isScrollingDown = true;
            onscreen = 0;
        }
        else
            bottomy = y;
        if (i >= this->scrollStart && onscreen)
            this->numVisible++;

        if (e->type == MENUVAR) {
            int valwidth = fontGetLen(((MenuEntry_Var*)e)->getValWidth());
            if (valwidth > maxValWidth)
                maxValWidth = valwidth;
        }
        if (e->r.right > maxNameWidth)
            maxNameWidth = e->r.right;
        i++;
    }
    if (this->r.right < maxNameWidth + maxValWidth + gap * fontscale)
        this->r.right = maxNameWidth + maxValWidth + gap * fontscale;

    this->scrollUpR = this->r;
    this->scrollUpR.bottom = 16;
    this->scrollDownR = this->scrollUpR;
    this->scrollDownR.y = bottomy;

    // Update active submenu
    if (this->selectedEntry && this->selectedEntry->type == MENUSUB) {
        Menu* submenu = ((MenuEntry_Sub*)this->selectedEntry)->submenu;
        submenu->r.left = this->r.left + this->r.right + 10;
        submenu->r.top = this->r.top;
        submenu->r.right = minwidth;	// update menu will expand
        submenu->r.bottom = this->r.bottom;
        submenu->update();
    }
}

void Menu::draw() {
    static char val[100];
    int i;
    MenuEntry* e;
    i = 0;
    for (e = this->entries; e; e = e->next) {
        if (i >= this->scrollStart + this->numVisible)
            break;
        if (i >= this->scrollStart) {
            int style = FONT_NORMAL;
            if (i == this->selection)
                style = this == activeMenu ? FONT_SEL_ACTIVE : FONT_SEL_INACTIVE;
            if (style != FONT_SEL_ACTIVE && e == mouseOverEntry)
                style = FONT_MOUSE;
            fontPrint(e->name, e->r.left, e->r.top, style);
            if (e->type == MENUVAR) {
                int valw = fontGetLen(((MenuEntry_Var*)e)->getValWidth());
                ((MenuEntry_Var*)e)->getValStr(val, 100);
                fontPrint(val, e->r.left + this->r.right - valw, e->r.top, style);
            }
        }
        i++;
    }

    if (this->isScrollingUp)
        drawArrow(this->scrollUpR, -1, isMouseInRect(this->scrollUpR));
    if (this->isScrollingDown)
        drawArrow(this->scrollDownR, 1, isMouseInRect(this->scrollDownR));

    if (this->selectedEntry && this->selectedEntry->type == MENUSUB)
        ((MenuEntry_Sub*)this->selectedEntry)->submenu->draw();
}

Menu* findMenu(const char* name) {
    Menu* m;
    MenuEntry* e;
    char* tmppath = strdup(name);
    char* next, * curname;

    curname = tmppath;
    next = curname;

    m = &toplevel;
    while (*next) {
        curname = next;
        while (*next) {
            if (*next == '|') {
                *next++ = '\0';
                break;
            }
            next++;
        }
        e = m->findEntry(curname);
        if (e) {
            // return an error if the entry exists but isn't a menu
            if (e->type != MENUSUB) {
                free(tmppath);
                return NULL;
            }
            m = ((MenuEntry_Sub*)e)->submenu;
        }
        else {
            // Create submenus that don't exist yet
            Menu* submenu = new Menu();
            submenu->parent = m;
            MenuEntry* me = new MenuEntry_Sub(curname, submenu);
            // Don't sort submenus outside the toplevel menu
            if (m == &toplevel)
                m->insertEntrySorted(me);
            else
                m->appendEntry(me);
            m = submenu;
        }
    }

    free(tmppath);
    return m;
}

/*
 * ****************
 * debug menu
 * ****************
 */

void processInput() {
    int shift = KEYDOWN(eKeyCodes::KEY_RSHIFT) || KEYDOWN(eKeyCodes::KEY_LSHIFT);
#define X(var, keycode) var = KEYJUSTDOWN(keycode);
    MUHKEYS
#undef X

        // Implement auto-repeat
#define X(var, keycode) \
	if(var){ \
		repeattime = downtime = CTimer::GetTimeInMilliseconds(); \
		lastkeydown = keycode; \
		keyptr = &var; \
	}
        MUHKEYS
#undef X
        if (lastkeydown) {
            if (KEYDOWN(lastkeydown)) {
                int curtime = CTimer::GetTimeInMilliseconds();
                if (curtime - downtime > REPEATDELAY) {
                    if (curtime - repeattime > REPEATINTERVAL) {
                        repeattime = curtime;
                        *keyptr = 1;
                    }
                }
            }
            else {
                lastkeydown = 0;
            }
        }

    // Also for mouse buttons
#define X(var, num) \
	if(var) { \
		repeattime = downtime = CTimer::GetTimeInMilliseconds(); \
		lastbuttondown = num; \
		buttonptr = &var; \
	}
    MUHBUTTONS
#undef X
        if (lastbuttondown) {
            if (buttondown[lastbuttondown - 1]) {
                int curtime = CTimer::GetTimeInMilliseconds();
                if (curtime - downtime > REPEATDELAY) {
                    if (curtime - repeattime > REPEATINTERVAL) {
                        repeattime = curtime;
                        *buttonptr = 1;
                    }
                }
            }
            else {
                lastbuttondown = 0;
            }
        }

    // Walk through all visible menus and figure out which one the mouse is over
    mouseOverMenu = NULL;
    mouseOverEntry = NULL;
    Menu* menu;
    for (menu = deepestMenu; menu; menu = menu->parent)
        if (isMouseInRect(menu->r)) {
            mouseOverMenu = menu;
            break;
        }
    if (mouseOverMenu) {
        // Walk all visibile entries and figure out which one the mouse is over
        MenuEntry* e;
        int i = 0;
        for (e = mouseOverMenu->entries; e; e = e->next) {
            if (i >= mouseOverMenu->scrollStart + mouseOverMenu->numVisible)
                break;
            if (i >= mouseOverMenu->scrollStart) {
                rage::Vector4 r = e->r;
                r.right = mouseOverMenu->r.right;	// span the whole menu
                if (isMouseInRect(r)) {
                    mouseOverEntry = e;
                    break;
                }
            }
            i++;
        }
        if (mouseOverMenu->isScrollingUp && isMouseInRect(mouseOverMenu->scrollUpR)) {
            mouseOverEntry = &scrollUpEntry;
            mouseOverEntry->r = mouseOverMenu->scrollUpR;
            mouseOverEntry->menu = mouseOverMenu;
            mouseOverEntry->type = MENUSCROLL;
        }
        if (mouseOverMenu->isScrollingDown && isMouseInRect(mouseOverMenu->scrollDownR)) {
            mouseOverEntry = &scrollDownEntry;
            mouseOverEntry->r = mouseOverMenu->scrollDownR;
            mouseOverEntry->menu = mouseOverMenu;
            mouseOverEntry->type = MENUSCROLL;
        }
    }

    if (pgupjustdown)
        activeMenu->scroll(shift ? -5 : -1);
    if (pgdnjustdown)
        activeMenu->scroll(shift ? 5 : 1);
    if (downjustdown)
        activeMenu->changeSelection(activeMenu->selection + (shift ? 5 : 1));
    if (upjustdown)
        activeMenu->changeSelection(activeMenu->selection - (shift ? 5 : 1));

    int32_t mouseWheel;
    CPad::GetMouseWheel(&mouseWheel);

    if (mouseWheel < 0) {
        if (mouseOverMenu)
            activeMenu = mouseOverMenu;
        activeMenu->scroll(shift ? -5 : -1);
    }
    if (mouseWheel > 0) {
        if (mouseOverMenu)
            activeMenu = mouseOverMenu;
        activeMenu->scroll(shift ? 5 : 1);
    }

    if (mouseOverEntry == &scrollUpEntry) {
        if (button1justdown) {
            activeMenu = mouseOverEntry->menu;
            activeMenu->scroll(shift ? -5 : -1);
        }
    }
    if (mouseOverEntry == &scrollDownEntry) {
        if (button1justdown) {
            activeMenu = mouseOverEntry->menu;
            activeMenu->scroll(shift ? 5 : 1);
        }
    }

    // Have to call this before processInput below because menu entry can change
    if ((button1justdown || button3justdown) && mouseOverEntry) {
        activeMenu = mouseOverEntry->menu;
        activeMenu->changeSelection(mouseOverEntry);
    }
    if (KEYJUSTDOWN(eKeyCodes::KEY_RETURN)) {
        if (activeMenu->selectedEntry && activeMenu->selectedEntry->type == MENUSUB)
            activeMenu = ((MenuEntry_Sub*)activeMenu->selectedEntry)->submenu;
    }
    else if (KEYJUSTDOWN(eKeyCodes::KEY_BACKSPACE)) {
        if (activeMenu->parent)
            activeMenu = activeMenu->parent;
    }
    else {
        if (mouseOverEntry && mouseOverEntry->type == MENUVAR)
            ((MenuEntry_Var*)mouseOverEntry)->processInput(true, mouseOverEntry == activeMenu->selectedEntry);
        if (activeMenu->selectedEntry && activeMenu->selectedEntry->type == MENUVAR &&
           mouseOverEntry != activeMenu->selectedEntry)
            ((MenuEntry_Var*)activeMenu->selectedEntry)->processInput(false, true);
    }
}

void updateMouse() {
    int dirX = 1;
    int dirY = 1;

    CPad::GetMousePos(&mouseX, &mouseY);

    mouseX *= SCREEN_WIDTH;
    mouseY *= SCREEN_HEIGHT;

    button1justdown = CPad::IsMouseButtonJustPressed(1);
    button2justdown = CPad::IsMouseButtonJustPressed(3);
    button3justdown = CPad::IsMouseButtonJustPressed(2);
    buttondown[0] = CPad::IsMouseButtonPressed(1);
    buttondown[1] = CPad::IsMouseButtonPressed(3);
    buttondown[2] = CPad::IsMouseButtonPressed(2);
}

EXPORT void DebugMenuInit() {
    if (menuInitialized == 1)
        return;

    int32_t slot = CTxdStore::AddTxdSlot("dbgmenu");
    CTxdStore::LoadTxd(slot, "platform:/textures/debugmenu");
    CTxdStore::AddRef(slot);
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(slot);

    arrowSprite.SetTexture("arrow");
    fontSprite.SetTexture("Bm437_IBM_VGA8");
    cursorSprite.SetTexture("cursor");

    CTxdStore::PopCurrentTxd();

    menuInitialized = 1;
}

EXPORT void DebugMenuShutdown() {
    if (menuInitialized == 0)
        return;

    arrowSprite.Delete();
    fontSprite.Delete();
    cursorSprite.Delete();

    int32_t slot = CTxdStore::FindTxdSlot("dbgmenu");
    CTxdStore::RemoveTxdSlot(slot);

    menuInitialized = 0;
}

EXPORT void DebugMenuProcess() {
    if (CTRLJUSTDOWN(eKeyCodes::KEY_M)) {
        menuOn ^= 1;

        auto playa = FindPlayerPed(0);
        auto cam = TheCamera.m_pCamGame;

        if (playa) {
            playa->m_pPlayerInfo->m_bDisableControls = menuOn;
            playa->m_pPlayerInfo->m_PlayerData.m_Wanted.m_bIgnoredByEveryone = menuOn;
        }

        if (cam)
            cam->m_bDisableControls = menuOn;
    }

    if (rage::screenHeight > 1080)
        fontscale = 2;
    else
        fontscale = 1;

    if (!menuOn)
        return;

    updateMouse();

    toplevel.update();
    processInput();
}

EXPORT void DebugMenuRender() {
    if (!menuOn)
        return;

    auto base = new T_CB_Generic([] {
        Pt sz;
        sz = fontPrint("Debug Menu", firstBorder * fontscale, topBorder, 0);

        toplevel.r.left = firstBorder * fontscale;
        toplevel.r.top = topBorder + sz.y + 10;
        toplevel.r.right = minwidth; // update menu will expand
        toplevel.r.bottom = SCREEN_HEIGHT - 10 - toplevel.r.top;
        toplevel.draw();

        drawMouse();

    });
    base->Append();
}

EXPORT bool DebugMenuShowing() {
    return menuOn;
}

EXPORT void DebugMenuPrintString(const char* str, float x, float y, int style) {
    fontPrint(str, x, y, style);
}

EXPORT int DebugMenuGetStringSize(const char* str) {
    return fontGetStringSize(str).x;
}

void drawArrow(rage::Vector4 r, int direction, int style) {
    SetRenderStates();
    int width = arrowSprite.m_pTexture->getWidth();
    int height = arrowSprite.m_pTexture->getHeight();

    float left = r.left + (r.right - width) / 2;
    float right = left + width;
    float top = r.top;
    float bottom = r.top + r.bottom;

    float umin = 0.5f / width;
    float vmin = 0.5f / height;
    float umax = (width + 0.5f) / width;
    float vmax = (height + 0.5f) / height;
    if (direction < 0) {
        vmin = (height - 0.5f) / height;
        vmax = -0.5f / height;
    }

    if (style) {
        CSprite2d::DrawRect({ left - 1, top - 1, right + 1, bottom + 1 }, { 132, 132, 132, 255 });
    }

    arrowSprite.Push();
    CSprite2d::Draw({ left, top, right, bottom }, { umin, vmin, umax, vmax }, { 255, 255, 255, 255 });
    CSprite2d::Pop();
    RestoreRenderStates();
}

void drawMouse() {
    SetRenderStates();
    float x = mouseX;
    float y = mouseY;
    float w = cursorSprite.m_pTexture->getWidth();
    float h = cursorSprite.m_pTexture->getHeight();

    cursorSprite.Push();
    CSprite2d::Draw(x, y, w, h, { 255, 255, 255, 255 });
    CSprite2d::Pop();
    RestoreRenderStates();
}

Pt fontPrint(const char* s, float xstart, float ystart, int style) {
    SetRenderStates();

    rage::Color32 col = { 225, 225, 225, 225 };
    rage::Color32 sel = { 132, 132, 132, 255 };
    rage::Color32 drop = { 0, 0, 0, 255 };

    switch (style) {
        case FONT_SEL_ACTIVE:
        case FONT_SEL_INACTIVE:
            col = { 200, 200, 200, 255 };
            break;
    }

    float u, v, du, dv;
    float uhalf, vhalf;
    float x, y;
    Pt sz;
    int szx;

    sz.y = fontGlyphHeight * fontscale;
    sz.x = 0;
    szx = 0;

    x = xstart;
    y = ystart;

    du = fontGlyphWidth / (float)fontSprite.m_pTexture->getWidth();
    dv = fontGlyphHeight / (float)fontSprite.m_pTexture->getHeight();
    uhalf = 0.5f / fontSprite.m_pTexture->getWidth();
    vhalf = 0.5f / fontSprite.m_pTexture->getHeight();

    Pt fntSize = fontGetStringSize(s);
    if (style == FONT_SEL_ACTIVE) {
        CSprite2d::DrawRect({ x - 1, y - 1, x + fntSize.x + 1, y + fntSize.y + 1 }, sel);
    }

    while (char c = *s++) {
        if (c == '\n') {
            x = xstart;
            y += fontGlyphHeight * fontscale;
            sz.y = fontGlyphHeight * fontscale;
            if (szx > sz.x)
                sz.x = szx;
            szx = 0;
            continue;
        }

        if (c >= fontNumGlyphs)
            c = 0;
        u = (c % 16) * fontGlyphWidth / (float)fontSprite.m_pTexture->getWidth();
        v = (c / 16) * fontGlyphHeight / (float)fontSprite.m_pTexture->getHeight();

        fontSprite.Push();
        if (fontShadow) {
            CSprite2d::Draw({ x + 1, y + 1, x + 1 + fontGlyphWidth * fontscale, y + 1 + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, drop);
        }
        if (fontOutline) {
            CSprite2d::Draw({ x - 1, y, x - 1 + fontGlyphWidth * fontscale, y + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, drop);
            CSprite2d::Draw({ x, y - 1, x + fontGlyphWidth * fontscale, y - 1 + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, drop);
            CSprite2d::Draw({ x, y + 1, x + fontGlyphWidth * fontscale, y + 1 + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, drop);
            CSprite2d::Draw({ x + 1, y, x + 1 + fontGlyphWidth * fontscale, y + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, drop);
        }
        CSprite2d::Draw({ x, y, x + fontGlyphWidth * fontscale, y + fontGlyphHeight * fontscale }, { u, v, u + du + uhalf, v + dv + vhalf }, col);
    
        CSprite2d::Pop();

        x += fontGlyphWidth * fontscale;
        szx += fontGlyphWidth * fontscale;
    }

    if (szx > sz.x)
        sz.x = szx;

    RestoreRenderStates();

    return sz;
}

Pt fontGetStringSize(const char* s) {
    Pt sz = { 0, 0 };
    int x;
    char c;
    sz.y = fontGlyphHeight * fontscale;	// always assume one line;
    x = 0;
    while (c = *s++) {
        if (c == '\n') {
            sz.y += fontGlyphHeight * fontscale;
            if (x > sz.x)
                sz.x = x;
            x = 0;
        }
        else
            x += fontGlyphWidth * fontscale;
    }
    if (x > sz.x)
        sz.x = x;
    return sz;
}

int fontGetLen(int len) {
    return len * fontGlyphWidth * fontscale;
}
