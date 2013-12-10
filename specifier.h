/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Copyright (C) Simon Gomizelj, 2013
*/

#pragma once

typedef int (*SpecifierCallback)(char specifier, void *data, void *userdata, char **ret);

typedef struct Specifier {
    const char specifier;
    const SpecifierCallback lookup;
    void *data;
} Specifier;

int specifier_user_pwd(char specifier, void *data, void *userdata, char **ret);
int specifier_string(char specifier, void *data, void *userdata, char **ret);

int specifier_printf(const char *text, const Specifier table[], void *userdata, char **env, char **_ret);
