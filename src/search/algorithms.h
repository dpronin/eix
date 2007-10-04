// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)                                                         
//   Wolfgang Frisch <xororand@users.sourceforge.net>                    
//   Emil Beinroth <emilbeinroth@gmx.net>                                
//   Martin V�th <vaeth@mathematik.uni-wuerzburg.de>                     

#ifndef __ALGORITHMS_H__
#define __ALGORITHMS_H__

/* FNM_CASEFOLD is a gnu extension .. */
#if !defined _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !defined _GNU_SOURCE */

#include <fnmatch.h>

#include <eixTk/levenshtein.h>
#include <eixTk/regexp.h>

#include <portage/package.h>

#include <cstring>

/* Check if we have FNM_CASEFOLD ..
 * fnmatch(3) tells me that this is a GNU extensions */
#if defined FNM_CASEFOLD
#define FNMATCH_FLAGS FNM_CASEFOLD
#else
#define FNMATCH_FLAGS 0
#endif /* defined FNM_CASEFOLD */

#define UNUSED(p) ((void)(p))

/** That's how every Algorithm will look like. */
class BaseAlgorithm {

	protected:
		std::string search_string;

	public:
		virtual void setString(std::string s) {
			search_string = s;
		}

		virtual ~BaseAlgorithm() {
			// Nothin' to see here, please move along
		}

		virtual bool operator () (const char *s, Package *p) = 0;
};

/** Use regex to test strings for a match. */
class RegexAlgorithm : public BaseAlgorithm {

	protected:
		Regex re;

	public:
		RegexAlgorithm()
		{ }

		void setString(std::string s) {
			search_string = s;
			re.compile(search_string.c_str());
		}

		bool operator () (const char *s, Package *p) {
			UNUSED(p);
			return re.match(s);
		}
};

/** Exact-string-matching, use strcmp to test for a match. */
class ESMAlgorithm : public BaseAlgorithm {

	public:
		bool operator () (const char *s, Package *p) {
			UNUSED(p);
			return !strcmp(search_string.c_str(), s);
		}
};

/** Store distance to searchstring in Package and sort out packages with a
 * higher distance than max_levenshteindistance. */
class FuzzyAlgorithm : public BaseAlgorithm {

	protected:
		int max_levenshteindistance;

		/** FIXME: We need to have a package->levenshtein mapping that we can
		 * access from the static FuzzyAlgorithm::compare.
		 * I really don't know how to do this .. */
		static std::map<std::string, int> levenshtein_map;

	public:
		FuzzyAlgorithm(int max) {
			max_levenshteindistance = max;
		}

		bool operator () (const char *s, Package *p) {
			int  d  = get_levenshtein_distance(search_string.c_str(), s);
			bool ok = (d <= max_levenshteindistance);
			if(ok)
			{
				if(p)
					levenshtein_map[p->category + "/" + p->name] = d;
			}
			return ok;
		}

		static bool compare(Package *p1, Package *p2)  {
			return (levenshtein_map[p1->category + "/" + p1->name]
					< levenshtein_map[p2->category + "/" + p2->name]);
		}

		static bool sort_by_levenshtein() {
			return levenshtein_map.size() > 0;
		}
};

/** Use fnmatch to test if the package matches. */
class WildcardAlgorithm : public BaseAlgorithm {

	public:
		bool operator () (const char *s, Package *p) {
			UNUSED(p);
			return !fnmatch(search_string.c_str(), const_cast<char *>(s), FNMATCH_FLAGS);
		}
};

#endif /* __ALGORITHMS_H__ */
