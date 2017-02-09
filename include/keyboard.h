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

#ifndef __TBF__KEYBOARD_H__
#define __TBF__KEYBOARD_H__

#include <set>
#include <vector>

typedef uint16_t SDL_Scancode;

namespace tbf
{
  namespace KeyboardFix
  {
    void Init     (void);
    void Shutdown (void);

    void UpdateConfig (void);

    void DrawControlPanel (void);
    bool RemapDialog      (void);

    extern std::set    <SDL_Scancode>                            remapped_scancodes;
    extern std::vector <std::pair <SDL_Scancode, SDL_Scancode> > swapped_keys;
  }
}

#endif /* __TBF__KEYBOARD_H__ */