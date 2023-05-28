// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_SEARCH_PACKAGETEST_H_
#define SRC_SEARCH_PACKAGETEST_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <set>
#include <string>
#include <vector>

#include "database/package_reader.h"
#include "eixTk/attribute.h"
#include "eixTk/dialect.h"
#include "eixTk/inttypes.h"
#include "eixTk/likely.h"
#include "eixTk/null.h"
#include "portage/extendedversion.h"
#include "portage/keywords.h"
#include "portage/package.h"
#include "portage/set_stability.h"
#include "search/redundancy.h"

class BaseAlgorithm;
class Mask;
class MatcherAlgorithm;
class MatcherField;
class NowarnMaskList;
class ParseError;
class PortageSettings;
class PrintFormat;
class VarDbPkg;
template<typename m_Type> class MaskList;

/**
Test a package if it matches some criteria
**/
class PackageTest {
		friend class MatcherField;
		friend class MatcherAlgorithm;

	public:
		typedef uint32_t MatchField;
		static CONSTEXPR const MatchField
			NONE          = 0x0000000U,  ///< Search in name
			NAME          = 0x0000001U,  ///< Search in name
			DESCRIPTION   = 0x0000002U,  ///< Search in description
			LICENSE       = 0x0000004U,  ///< Search in license
			CATEGORY      = 0x0000008U,  ///< Search in category
			CATEGORY_NAME = 0x0000010U,  ///< Search in category/name
			HOMEPAGE      = 0x0000020U,  ///< Search in homepage
			IUSE          = 0x0000040U,  ///< Search in iuse
			USE_ENABLED   = 0x0000080U,  ///< Search in enabled  useflags of installed packages
			USE_DISABLED  = 0x0000100U,  ///< Search in disabled useflags of installed packages
			SRC_URI       = 0x0000200U,  ///< Search in SRC_URI
			EAPI          = 0x0000400U,  ///< Search in EAPI
			INST_EAPI     = 0x0000800U,  ///< Search in installed EAPI
			SLOT          = 0x0001000U,  ///< Search in slots
			FULLSLOT      = 0x0002000U,  ///< Search in full slots
			INST_SLOT     = 0x0004000U,  ///< Search in installed slots
			INST_FULLSLOT = 0x0008000U,  ///< Search in installed full slots
			SET           = 0x0010000U,  ///< Search in sets
			DEPENDA       = 0x0020000U,  ///< Search in DEPEND (available)
			RDEPENDA      = 0x0040000U,  ///< Search in RDEPEND (available)
			PDEPENDA      = 0x0080000U,  ///< Search in PDEPEND (available)
			BDEPENDA      = 0x0100000U,  ///< Search in BDEPEND (available)
			IDEPENDA      = 0x0200000U,  ///< Search in IDEPEND (available)
			DEPENDI       = 0x0400000U,  ///< Search in DEPEND (installed)
			RDEPENDI      = 0x0800000U,  ///< Search in RDEPEND (installed)
			PDEPENDI      = 0x1000000U,  ///< Search in PDEPEND (installed)
			BDEPENDI      = 0x2000000U,  ///< Search in BDEPEND (installed)
			IDEPENDI      = 0x2000000U,  ///< Search in IDEPEND (installed)
			DEPEND        = DEPENDA | DEPENDI,
			RDEPEND       = RDEPENDA | RDEPENDI,
			PDEPEND       = PDEPENDA | PDEPENDI,
			BDEPEND       = BDEPENDA | BDEPENDI,
			IDEPEND       = IDEPENDA | IDEPENDI,
			DEPSA         = DEPENDA | RDEPENDA | PDEPENDA | BDEPENDA | IDEPENDA,
			DEPSI         = DEPENDI | RDEPENDI | PDEPENDI | BDEPENDI | IDEPENDI,
			DEPS          = DEPSA | DEPSI,
			ANY           = NAME | DESCRIPTION | LICENSE | CATEGORY | CATEGORY_NAME | HOMEPAGE | IUSE | USE_ENABLED | USE_DISABLED | SRC_URI | EAPI | INST_EAPI | SLOT | FULLSLOT | INST_SLOT | INST_FULLSLOT | SET | DEPS;

		enum MatchAlgorithm {
			ALGO_REGEX,
			ALGO_REGEXCASE,
			ALGO_EXACT,
			ALGO_BEGIN,
			ALGO_END,
			ALGO_SUBSTRING,
			ALGO_PATTERN,
			ALGO_FUZZY,
			ALGO_ERROR
		};

		typedef uint8_t TestInstalled;
		static CONSTEXPR const TestInstalled
			INS_NONE        = 0x00U,
			INS_NONEXISTENT = 0x01U,  ///< Test for nonexistent installed packages
			INS_OVERLAY     = 0x02U,  ///< Test for nonexistent overlays of installed packages
			INS_MASKED      = 0x04U,  ///< Test for masked installed packages
			INS_SOME        = INS_NONEXISTENT | INS_OVERLAY | INS_MASKED;

		typedef uint8_t TestStability;
		static CONSTEXPR const TestStability
			STABLE_NONE         = 0x00U,
			STABLE_FULL         = 0x01U,  ///< Test for stable keyword
			STABLE_TESTING      = 0x02U,  ///< Test for testing keyword
			STABLE_NONMASKED    = 0x04U,  ///< Test for non-masked packages
			STABLE_SYSTEM       = 0x08U,  ///< Test for system packages
			STABLE_PROFILE      = 0x10U,  ///< Test for stable packages
			STABLE_SYSTEMPROFILE = STABLE_SYSTEM | STABLE_PROFILE;

		/**
		Set default values.
		**/
		ATTRIBUTE_NONNULL_ PackageTest(VarDbPkg *vdb, PortageSettings *p, const PrintFormat *f, const SetStability *stability, const DBHeader *dbheader, const ParseError *e);

		~PackageTest();

		void setAlgorithm(BaseAlgorithm *p);

		void setAlgorithm(MatchAlgorithm a);

		ATTRIBUTE_NONNULL_ void setPattern(const char *p);

		bool match(PackageReader *pkg) const;

		/**
		Set defaults (e.g. matchfield if unspecified), calculate needs
		**/
		void finalize();

		/*
		The constructor of the class *must* set the least restrictive choice.
		Since --selected --world must act like --selected, the less restrictive
		choice (here --selected) must change the variable unconditionally,
		and so the default set in the constructor must have been opposite.
		We thus name the variables such that the less restrictive version
		corresponds to false/NULLPTR and let the constructor set all to false/NULLPTR.
		*/

		void Installed() {
			installed = true;
		}

		void MultiInstalled() {
			installed = multi_installed = true;
		}

		void Slotted() {
			slotted = true;
		}

		void MultiSlotted() {
			slotted = multi_slot = true;
		}

		void Upgrade(LocalMode local_mode) {
			upgrade = true;
			upgrade_local_mode = local_mode;
		}

		void SetStabilityDefault(TestStability require) {
			test_stability_default |= require;
		}

		void SetStabilityLocal(TestStability require) {
			test_stability_local |= require;
		}

		void SetStabilityNonlocal(TestStability require) {
			test_stability_nonlocal |= require;
		}

		void SetInstability(TestStability avoid) {
			test_instability |= avoid;
		}

		void SetMarkedList(MaskList<Mask> *markedlist) {
			marked_list = markedlist;
		}

		void Virtual() {
			have_virtual = true;
		}

		void Nonvirtual() {
			have_nonvirtual = true;
		}

		void Overlay() {
			overlay = true;
		}

		void Restrictions(ExtendedVersion::Restrict flags) {
			restrictions |= flags;
		}

		void Properties(ExtendedVersion::Properties flags) {
			properties |= flags;
		}

		void Binary(ExtendedVersion::CountBinPkg num) {
			binarynum = num;
		}

		void WorldAll() {
			world = true;
		}

		void WorldFile() {
			world = world_only_file = true;
		}

		void SelectedAll() {
			world = world_only_selected = true;
		}

		void SelectedFile() {
			world = world_only_selected = world_only_file = true;
		}

		void WorldSet() {
			worldset = true;
		}

		void SelectedSet() {
			worldset = worldset_only_selected = true;
		}

		ATTRIBUTE_NONNULL_ void StabilityDefault(Package *p) const {
			stability->set_stability(p);
		}

		ATTRIBUTE_NONNULL_ void StabilityLocal(Package *p) const {
			stability->set_stability(true, p);
		}

		ATTRIBUTE_NONNULL_ void StabilityNonlocal(Package *p) const {
			stability->set_stability(false, p);
		}

		std::set<ExtendedVersion::Overlay> *OverlayList() {
			if(likely(overlay_list == NULLPTR))
				overlay_list = new std::set<ExtendedVersion::Overlay>;
			return overlay_list;
		}

		std::set<ExtendedVersion::Overlay> *OverlayOnlyList() {
			if(likely(overlay_only_list == NULLPTR))
				overlay_only_list = new std::set<ExtendedVersion::Overlay>;
			return overlay_only_list;
		}

		std::set<ExtendedVersion::Overlay> *InOverlayInstList() {
			if(likely(in_overlay_inst_list == NULLPTR))
				in_overlay_inst_list = new std::set<ExtendedVersion::Overlay>;
			return in_overlay_inst_list;
		}

		std::set<ExtendedVersion::Overlay> *FromOverlayInstList() {
			if(likely(from_overlay_inst_list == NULLPTR))
				from_overlay_inst_list = new std::set<ExtendedVersion::Overlay>;
			return from_overlay_inst_list;
		}

		std::vector<std::string> *FromForeignOverlayInstList() {
			if(likely(from_foreign_overlay_inst_list == NULLPTR))
				from_foreign_overlay_inst_list = new std::vector<std::string>;
			return from_foreign_overlay_inst_list;
		}

		void DuplVersions(bool only_overlay) {
			dup_versions = true;
			dup_versions_overlay = only_overlay;
		}

		void DuplPackages(bool only_overlay) {
			dup_packages = true;
			dup_packages_overlay = only_overlay;
		}

		void ObsoleteCfg(const RedAtom& first, const RedAtom& second, TestInstalled test_ins) {
			obsolete           = true;
			redundant_flags    = first.red|second.red;
			first_test         = first;
			second_test        = second;
			test_installed     = test_ins;
		}

		MatchField operator|=(const MatchField m) {
			return field |= m;
		}

		MatchField operator=(const MatchField m) {
			return field = m;
		}

		static void init_static();

	private:
		static NowarnMaskList *nowarn_list;
		void get_nowarn_list() const;

		/**
		What to match
		**/
		MatchField field;
		/**
		Lookup stuff about installed packages here
		**/
		VarDbPkg *vardbpkg;
		/**
		Check virtual overlays with the aid of this
		**/
		const PrintFormat *print_format;
		/**
		When reading overlay information use this
		**/
		const DBHeader *header;

		const ParseError *parse_error;

		/**
		What we need to read so we can do our testing
		**/
		PackageReader::Attributes need;
		/**
		Our string matching algorithm
		**/
		BaseAlgorithm *algorithm;

		/**
		Other flags for tests
		**/
		bool	overlay, obsolete, upgrade,
			installed, multi_installed,
			slotted, multi_slot,
			world, world_only_file, world_only_selected,
			worldset, worldset_only_selected,
			have_virtual, have_nonvirtual,
			know_pattern;
		LocalMode upgrade_local_mode;
		bool dup_versions, dup_versions_overlay;
		bool dup_packages, dup_packages_overlay;
		ExtendedVersion::CountBinPkg binarynum;
		ExtendedVersion::Restrict restrictions;
		ExtendedVersion::Properties properties;

		std::set<ExtendedVersion::Overlay>
			*overlay_list, *overlay_only_list,
			*in_overlay_inst_list;

		std::set<ExtendedVersion::Overlay> *from_overlay_inst_list;
		std::vector<std::string> *from_foreign_overlay_inst_list;

		MaskList<Mask> *marked_list;

		PortageSettings *portagesettings;
		/**
		Lookup stuff about user flags here
		**/
		const SetStability *stability,
			*stability_local, *stability_nonlocal;
		/**
		Test for this redundancy
		**/
		Keywords::Redundant redundant_flags;
		RedAtom first_test, second_test;
		TestInstalled test_installed;
		TestStability test_stability_default,
			test_stability_local, test_stability_nonlocal;
		TestStability test_instability;

		static MatchField     name2field(const std::string& p, bool default_match_field);
		static MatchAlgorithm name2algorithm(const std::string& p);
		static MatchField     get_matchfield(const char *p);
		static MatchAlgorithm get_matchalgorithm(const char *p, MatchField field);
		static void parse_field_specification(const std::string& spec, MatchField *or_field, MatchField *and_field, MatchField *not_field);

		ATTRIBUTE_NONNULL_ bool stringMatch(Package *pkg) const;

		void setNeeds(const PackageReader::Attributes i) {
			if(need < i) {
				need = i;
			}
		}

		/**
		Get the Fetched-value that is required to determine the match
		**/
		void calculateNeeds();

		bool have_redundant(const Package& p, Keywords::Redundant r, const RedAtom& t) const;
		bool have_redundant(const Package& p, Keywords::Redundant r) const;
		ATTRIBUTE_NONNULL_ bool instabilitytest(const Package *p, TestStability what) const;

		static Keywords::Redundant nowarn_keywords(const Package& p);
		static Keywords::Redundant nowarn_mask(const Package& p);
		static Keywords::Redundant nowarn_use(const Package& p);
		static Keywords::Redundant nowarn_env(const Package& p);
		static Keywords::Redundant nowarn_cflags(const Package& p);
		static TestInstalled nowarn_installed(const Package& p);
};

#endif  // SRC_SEARCH_PACKAGETEST_H_
