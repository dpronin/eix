/***************************************************************************
 *   eix is a small utility for searching ebuilds in the                   *
 *   Gentoo Linux portage system. It uses indexing to allow quick searches *
 *   in package descriptions with regular expressions.                     *
 *                                                                         *
 *   https://sourceforge.net/projects/eix                                  *
 *                                                                         *
 *   Copyright (c)                                                         *
 *     Wolfgang Frisch <xororand@users.sourceforge.net>                    *
 *     Emil Beinroth <emilbeinroth@gmx.net>                                *
 *     Martin V�th <vaeth@mathematik.uni-wuerzburg.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __CACHEUTILS_H__
#define __CACHEUTILS_H__

#include <eixTk/exceptions.h>
#include <portage/keywords.h>
#include <dirent.h>
#include <map>
#include <string>

class Package;
class Version;

int package_selector (SCANDIR_ARG3 dent);
int ebuild_selector (SCANDIR_ARG3 dent);
void flat_get_keywords_slot(const std::string &filename, std::string &keywords, std::string &slot) throw (ExBasic);
void flat_read_file(const char *filename, Package *pkg) throw (ExBasic);
void env_add_package(std::map<std::string,std::string> &env, const Package &package, const Version &version, const char *ebuild_name);

#endif /* __CACHEUTILS_H__ */
