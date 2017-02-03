/**
 * This file is part of Tales of Berseria "Fix".
 *
 * Tales of Berseria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Berseria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Berseria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __TBF__INPUT_H__
#define __TBF__INPUT_H__

struct SDL_Joystick;

struct SDL_JoystickGUID {
  uint8_t data [16];
};

namespace tbf
{
  namespace InputFix
  {
    void Init     ();
    void Shutdown ();
    
    struct ai_fix_s {
      struct virtual_joys {
        SDL_Joystick*    handle = (SDL_Joystick *)(LPVOID)0xDEADBEEFULL;
        SDL_JoystickGUID guid { 0xff };
      } joysticks [8];

    } extern ai_fix;
  };
};


#endif /* __TBF__INPUT_H__ */