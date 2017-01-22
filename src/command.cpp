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
#include "command.h"

extern HMODULE hInjectorDLL;

typedef SK_ICommandProcessor* (__stdcall *SK_GetCommandProcessor_pfn)
  ( void );

typedef SK_IVariable*         (__stdcall *SK_CreateVar_pfn)
  ( SK_IVariable::VariableType  type,
    void*                       var,
    SK_IVariableListener       *pListener );

SK_ICommandProcessor*
__stdcall
SK_GetCommandProcessor (void)
{

  static SK_GetCommandProcessor_pfn
    SK_GetCommandProcessor_imp =
      (SK_GetCommandProcessor_pfn)
        GetProcAddress ( hInjectorDLL,
                           "SK_GetCommandProcessor" );

  return SK_GetCommandProcessor_imp ();
}

SK_IVariable*
TBF_CreateVar ( SK_IVariable::VariableType  type,
                void*                       var,
                SK_IVariableListener       *pListener )
{
  static SK_CreateVar_pfn
    SK_CreateVar =
      (SK_CreateVar_pfn)
        GetProcAddress ( hInjectorDLL,
                           "SK_CreateVar" );

  return SK_CreateVar (type, var, pListener);
}