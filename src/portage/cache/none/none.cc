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

#include "none.h"

#include <varsreader.h>
#include <portage/package.h>
#include <portage/version.h>
#include <portage/packagetree.h>

#include <dirent.h>

#include <config.h>

using namespace std;

static int package_selector (SCANDIR_ARG3 dent)
{
	return (dent->d_name[0] != '.'
			&& strcmp(dent->d_name, "CVS") != 0);
}

static int ebuild_selector (SCANDIR_ARG3 dent)
{
		return package_selector(dent);
}

void NoneCache::readPackage(Category &vec, char *pkg_name, string *directory_path, struct dirent **list, int numfiles)
{
	// Make sure to no allocate for a nonexistent package:
	if(numfiles<=0)
		return;

	Package *pkg = vec.findPackage(pkg_name);
	if( !pkg )
		pkg = vec.addPackage(pkg_name);

	for(int i = 0; i<numfiles; ++i)
	{
		/* Check if this is an ebuild  */
		char* dotptr = strrchr(list[i]->d_name, '.');
		if( !(dotptr) || strcmp(dotptr,".ebuild") )
			continue;

		/* For splitting the version, we cut off the .ebuild */
		*dotptr = '\0';
		/* Check if we can split it */
		char* ver = ExplodeAtom::split_version(list[i]->d_name);
		/* Restore the old value */
		*dotptr = '.';
		if(ver == NULL) {
			m_error_callback("Can't split filename of ebuild %s/%s.", directory_path->c_str(), list[i]->d_name);
			continue;
		}

		/* Make version and add it to package. */
		Version *version = new Version(ver);
		pkg->addVersion(version);
		/* For the latest version read/change corresponding data */
		bool read_all = (*(pkg->latest()) == *version);
		/* read the ebuild */
		VarsReader ebuild((read_all ? VarsReader::NONE : VarsReader::ONLY_KEYWORDS));
		ebuild.read((*directory_path + "/" + list[i]->d_name).c_str());
		version->overlay_key = m_overlay_key;
		version->set(m_arch, ebuild["KEYWORDS"]);
		if(read_all)
		{
			pkg->homepage = ebuild["HOMEPAGE"];
			pkg->licenses = ebuild["LICENSE"];
			pkg->desc     = ebuild["DESCRIPTION"];
			pkg->provide  = ebuild["PROVIDE"];
		}
		free(ver);
	}
}

int NoneCache::readCategory(Category &vec) throw(ExBasic)
{
	struct dirent **packages= NULL;

	string catpath = m_scheme + "/" + vec.name();
	int numpackages = scandir(catpath.c_str(),
			&packages, package_selector, alphasort);

	for(int i = 0; i<numpackages; ++i)
	{
		struct dirent **files = NULL;
		string pkg_path = catpath + "/" + packages[i]->d_name;

		int numfiles = scandir(pkg_path.c_str(),
				&files, ebuild_selector, alphasort);
		if(numfiles > 0)
		{
			readPackage(vec, (char *) packages[i]->d_name, &pkg_path, files, numfiles);
			for(int i=0; i<numfiles; i++ )
				free(files[i]);
			free(files);
		}
	}

	for(int i=0; i<numpackages; i++ )
		free(packages[i]);
	if(numpackages > 0)
		free(packages);

	return 1;
}
