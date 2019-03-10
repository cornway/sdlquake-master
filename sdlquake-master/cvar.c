/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;
char	*cvar_null_string = "";

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
	
	for (var=cvar_vars ; var ; var=CVAR_NEXT(var))
		if (!Q_strcmp (var_name, CVAR_NAME(var)))
			return var;
	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (CVAR_NAME(var));
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (var)
        return CVAR_NAME(var);

    return cvar_null_string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cvar=cvar_vars ; cvar ; cvar=CVAR_NEXT(cvar))
		if (!Q_strncmp (partial,CVAR_NAME(cvar), len))
			return CVAR_NAME(cvar);
	return NULL;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set (char *var_name, char *value)
{
	cvar_t	*var;
	qboolean changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
	}

	changed = Q_strcmp(CVAR_NAME(var), value);
#if CVAR_TINY
    Sys_Error("CVAR_TINY : Z_Malloc");
#endif
	Z_Free (CVAR_NAME(var));	// free the old value string
	CVAR_NAME(var) = Z_Malloc (Q_strlen(value)+1);
	Q_strcpy (CVAR_NAME(var), value);
	var->value = Q_atof (CVAR_NAME(var));
	if (CVAR_SERVER(var) && changed)
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", CVAR_NAME(var), CVAR_STRING(var));
	}
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];
	
	sprintf (val, "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
#if CVAR_TINY
void Cvar_RegisterVariable (cvar_t *variable)
{
}
#else
void Cvar_RegisterVariable (cvar_t *variable)
{
	char	*oldstr;
	
// first check to see if it has allready been defined
	if (Cvar_FindVar (CVAR_NAME(variable)))
	{
		Con_Printf ("Can't register variable %s, allready defined\n",CVAR_NAME(variable));
		return;
	}
	
// check for overlap with a command
	if (Cmd_Exists (CVAR_NAME(variable)))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", CVAR_NAME(variable));
		return;
	}
		
// copy the value off, because future sets will Z_Free it
	oldstr = CVAR_STRING(variable);
	CVAR_STRING(variable) = Z_Malloc (Q_strlen(CVAR_STRING(variable))+1);	
	Q_strcpy (CVAR_STRING(variable), oldstr);
	CVAR_VALUE(variable) = Q_atof (CVAR_STRING(variable));
	
// link the variable in
	CVAR_NEXT(variable) = cvar_vars;
	cvar_vars = variable;
}
#endif /*CVAR_TINY*/

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", CVAR_NAME(v), CVAR_STRING(v));
		return true;
	}

	Cvar_Set (CVAR_NAME(v), Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (int handle)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = CVAR_NEXT(var))
		if (CVAR_ARCH(var))
			Sys_FPrintf(handle, "%s \"%s\"\n", CVAR_NAME(var), CVAR_STRING(var));
}

