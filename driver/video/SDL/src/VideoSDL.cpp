// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "main.h" // IWYU pragma: keep
#include "VideoSDL.h"
#ifndef RTTR_Assert
#   define RTTR_Assert assert
#endif
#include "../../../../src/helpers/containerUtils.h"
#include "VideoDriverLoaderInterface.h"
#include <VideoInterface.h>
#include <build_version.h>

#include <SDL.h>
#include <algorithm>

#ifdef _WIN32
#include "../../../../win32/resource.h"
#include <SDL_syswm.h>
#endif // _WIN32

/**
 *  Instanzierungsfunktion von @p VideoSDL.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 */
DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoSDL(CallBack);
}

DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

/**
 *  Namensfunktion von @p VideoSDL.
 *
 *  @return liefert den Namen des Treibers.
 */
DRIVERDLLAPI const char* GetDriverName(void)
{
    return "(SDL) OpenGL via SDL-Library";
}

/** @class VideoSDL
 *
 *  Klasse für den SDL Videotreiber.
 */

/** @var VideoSDL::screen
 *
 *  Das Fenster-SDL-Surface.
 */

/**
 *  Konstruktor von @p VideoSDL.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoSDL::VideoSDL(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), screen(NULL)
{}

VideoSDL::~VideoSDL()
{
    CleanUp();
}

/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 */
const char* VideoSDL::GetName() const
{
    return GetDriverName();
}

/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::Initialize()
{
    if( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        initialized = false;
        return false;
    }

    initialized = true;

    // Unicode-Support einschalten
    SDL_EnableUNICODE(1);

    // Key-Repeat einschalten
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    return initialized;
}

/**
 *  Treiberaufräumfunktion.
 */
void VideoSDL::CleanUp()
{
    if(!initialized)
        return;

    // Fenster vernichten
    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    // nun sind wir nicht mehr initalisiert
    initialized = false;
}

/**
 *  Erstellt das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::CreateScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    char title[512];

    if(!initialized)
        return false;

    // TODO: Icon setzen


    // GL-Attribute setzen
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

#ifdef _WIN32
    // das spinnt ja total unter windows ...
    this->isFullscreen_ = false;
#else
    this->isFullscreen_ = fullscreen;
#endif

    // Videomodus setzen
    if(!(screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL | (this->isFullscreen_ ? SDL_FULLSCREEN : SDL_RESIZABLE))))
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }

    sprintf(title, "%s - v%s-%s", GetWindowTitle(), GetWindowVersion(), GetWindowRevision());
    SDL_WM_SetCaption(title, 0);

#ifdef _WIN32
    SetWindowTextA(GetConsoleWindow(), title);
#endif

    std::fill(keyboard.begin(), keyboard.end(), false);

    this->screenWidth  = width;
    this->screenHeight = height;

    SDL_ShowCursor(SDL_DISABLE);

    return true;
}

/**
 *  Erstellt oder verändert das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @todo Vollbildmodus ggf. wechseln
 */
bool VideoSDL::ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    if(!initialized)
        return false;

    this->screenWidth  = width;
    this->screenHeight = height;

#ifdef _WIN32
    // das spinnt ja total unter windows ...
    this->isFullscreen_ = false;
#else
    this->isFullscreen_ = fullscreen;
#endif

    // Videomodus setzen
    if(!(screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL | (this->isFullscreen_ ? SDL_FULLSCREEN : SDL_RESIZABLE))))
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }

    return true;
}

/**
 *  Schliesst das Fenster.
 */
void VideoSDL::DestroyScreen()
{
    // Fenster schliessen
    CleanUp();
    Initialize();
}

/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::SwapBuffers()
{
    // Puffer wechseln
    SDL_GL_SwapBuffers();

    return true;
}

/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::MessageLoop()
{
    SDL_Event ev;

    static bool mouse_motion = 0;

    while(SDL_PollEvent(&ev))
    {
        switch(ev.type)
        {
            default:
                break;

            case SDL_QUIT:
                return false;

            case SDL_ACTIVEEVENT:
                if((ev.active.state & SDL_APPACTIVE) && ev.active.gain)
                {
                    // Window was restored. We need a resize to avoid a black screen
                    CallBack->ScreenResized(screenWidth, screenHeight);
                }
                break;

            case SDL_VIDEORESIZE:
            {
                ResizeScreen(ev.resize.w, ev.resize.h, isFullscreen_);
                CallBack->ScreenResized(screenWidth, screenHeight);
            } break;

            case SDL_KEYDOWN:
            {
                KeyEvent ke = { KT_INVALID, 0, false, false, false };

                switch(ev.key.keysym.sym)
                {
                    default:
                    {
                        // Die 12 F-Tasten
                        if(ev.key.keysym.sym >= SDLK_F1 && ev.key.keysym.sym <= SDLK_F12)
                            ke.kt = static_cast<KeyType>(KT_F1 + ev.key.keysym.sym - SDLK_F1);
                    } break;
                    case SDLK_RETURN:    ke.kt = KT_RETURN; break;
                    case SDLK_SPACE:     ke.kt = KT_SPACE; break;
                    case SDLK_LEFT:      ke.kt = KT_LEFT; break;
                    case SDLK_RIGHT:     ke.kt = KT_RIGHT; break;
                    case SDLK_UP:        ke.kt = KT_UP; break;
                    case SDLK_DOWN:      ke.kt = KT_DOWN; break;
                    case SDLK_BACKSPACE: ke.kt = KT_BACKSPACE; break;
                    case SDLK_DELETE:    ke.kt = KT_DELETE; break;
                    case SDLK_LSHIFT:    ke.kt = KT_SHIFT; break;
                    case SDLK_RSHIFT:    ke.kt = KT_SHIFT; break;
                    case SDLK_TAB:       ke.kt = KT_TAB; break;
                    case SDLK_HOME:      ke.kt = KT_HOME; break;
                    case SDLK_END:       ke.kt = KT_END; break;
                    case SDLK_ESCAPE:    ke.kt = KT_ESCAPE; break;
                    case SDLK_BACKQUOTE: ev.key.keysym.unicode = '^'; break;
                }

                /// Strg, Alt, usw gedrückt?
                if(ev.key.keysym.mod & KMOD_CTRL) ke.ctrl = true;
                if(ev.key.keysym.mod & KMOD_SHIFT) ke.shift = true;
                if(ev.key.keysym.mod & KMOD_ALT) ke.alt = true;

                if(ke.kt == KT_INVALID)
                {
                    ke.kt = KT_CHAR;
                    ke.c = ev.key.keysym.unicode;
                }

                CallBack->Msg_KeyDown(ke);
            } break;
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_xy.x = ev.button.x;
                mouse_xy.y = ev.button.y;

                if(/*!mouse_xy.ldown && */ev.button.button == SDL_BUTTON_LEFT)
                {
                    mouse_xy.ldown = true;
                    CallBack->Msg_LeftDown(mouse_xy);
                }
                if(/*!mouse_xy.rdown &&*/ ev.button.button == SDL_BUTTON_RIGHT)
                {
                    mouse_xy.rdown = true;
                    CallBack->Msg_RightDown(mouse_xy);
                }
            } break;
            case SDL_MOUSEBUTTONUP:
            {
                mouse_xy.x = ev.button.x;
                mouse_xy.y = ev.button.y;

                if(/*mouse_xy.ldown &&*/ ev.button.button == SDL_BUTTON_LEFT)
                {
                    mouse_xy.ldown = false;
                    CallBack->Msg_LeftUp(mouse_xy);
                }
                if(/*mouse_xy.rdown &&*/ ev.button.button == SDL_BUTTON_RIGHT)
                {
                    mouse_xy.rdown = false;
                    CallBack->Msg_RightUp(mouse_xy);
                }
                if(ev.button.button == SDL_BUTTON_WHEELUP)
                {
                    CallBack->Msg_WheelUp(mouse_xy);
                }
                if(ev.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    CallBack->Msg_WheelDown(mouse_xy);
                }

            } break;
            case SDL_MOUSEMOTION:
            {
                if(!mouse_motion)
                {
                    mouse_xy.x = ev.motion.x;
                    mouse_xy.y = ev.motion.y;

                    mouse_motion = 1;
                    CallBack->Msg_MouseMove(mouse_xy);
                }


            } break;
        }
    }

    mouse_motion = 0;
    return true;
}

/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 */
unsigned long VideoSDL::GetTickCount() const
{
    return SDL_GetTicks();
}

/**
 *   Listet verfügbare Videomodi auf
 *
 *  @param[in,out] video_modes Der Vector mit den Videomodes
 */
void VideoSDL::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);

    for (unsigned int i = 0; modes[i]; ++i)
    {
        VideoMode vm(modes[i]->w, modes[i]->h);
        if(!helpers::contains(video_modes, vm))
            video_modes.push_back(vm);
    }
}

/**
 *  Funktion zum Holen einer Subfunktion.
 *
 *  @param[in] function Name der Funktion welche geholt werden soll.
 *
 *  @return Adresse der Funktion bei Erfolg, @p NULL bei Fehler
 */
void* VideoSDL::GetFunction(const char* function) const
{
    return SDL_GL_GetProcAddress(function);
}

/**
 *  Funktion zum Setzen der Mauskoordinaten.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 */
void VideoSDL::SetMousePos(int x, int y)
{
    mouse_xy.x = x;
    mouse_xy.y = y;
    SDL_WarpMouse(x, y);
}

/**
 *  Funktion zum Setzen der X-Koordinate der Maus.
 *
 *  @param[in] x X-Koordinate
 */
void VideoSDL::SetMousePosX(int x)
{
    SetMousePos(x, mouse_xy.y);
}

/**
 *  Funktion zum Setzen der Y-Koordinate der Maus.
 *
 *  @param[in] y Y-Koordinate
 */
void VideoSDL::SetMousePosY(int y)
{
    SetMousePos(mouse_xy.x, y);
}

/**
 *  Get state of the modifier keys
 */
KeyEvent VideoSDL::GetModKeyState() const
{
    const SDLMod modifiers = SDL_GetModState();
    const KeyEvent ke = { KT_INVALID, 0, ( (modifiers& KMOD_CTRL) != 0), ( (modifiers& KMOD_SHIFT) != 0), ( (modifiers& KMOD_ALT) != 0)};
    return ke;
}

/// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
void* VideoSDL::GetMapPointer() const
{
#ifdef WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWMInfo(&wmInfo);
    //return (void*)wmInfo.info.win.window;
    return (void*)wmInfo.window;
#else
    return NULL;
#endif
}
