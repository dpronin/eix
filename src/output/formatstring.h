// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_OUTPUT_FORMATSTRING_H_
#define SRC_OUTPUT_FORMATSTRING_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <sys/types.h>

#include <stack>
#include <string>
#include <vector>

#include "database/header.h"
#include "eixTk/assert.h"
#include "eixTk/attribute.h"
#include "eixTk/null.h"
#include "eixTk/outputstring.h"
#include "eixTk/unordered_map.h"
#include "portage/extendedversion.h"
#include "portage/package.h"
#include "portage/set_stability.h"

class DBHeader;
class EixRc;
class IUseSet;
class KeywordsFlags;
class Mask;
class MaskFlags;
class Package;
class PortageSettings;
class VarDbPkg;
class Version;
template<typename m_Type> class MaskList;

class Node {
	public:
		enum Type { TEXT, OUTPUT, SET, IF } type;
		Node *next;

		explicit Node(Type t) : type(t), next(NULLPTR) {
		}

		/**
		Virtual deconstructor
		**/
		virtual ~Node() {
			delete next;
		}
};

class Text : public Node {
	public:
		OutputString text;

		Text() : Node(TEXT) {
		}

		explicit Text(const OutputString& t) : Node(TEXT), text(t) {
		}

		Text(const std::string& t, std::string::size_type s) : Node(TEXT), text(t, s) {
		}
};

class Property : public Node {
	public:
		std::string name;
		bool user_variable;

		Property() : Node(OUTPUT), user_variable(false) {
		}

		explicit Property(const std::string& n) : Node(OUTPUT), name(n), user_variable(false) {
		}

		Property(const std::string& n, bool user_var) : Node(OUTPUT), name(n), user_variable(user_var) {
		}
};

class ConditionBlock : public Node {
	public:
		bool final;

		Property variable;
		Text     text;
		enum Rhs { RHS_STRING, RHS_PROPERTY, RHS_VAR } rhs;
		Node     *if_true, *if_false;
		bool user_variable, negation;

		ConditionBlock() : Node(IF), final(false), if_true(NULLPTR), if_false(NULLPTR) {
		}

		~ConditionBlock() {
			delete if_true;
			delete if_false;
		}
};

class FormatParser {
	private:
		enum ParserState {
			ERROR, STOP, START,
			TEXT, COLOR, PROPERTY,
			IF, ELSE, FI
		};

		std::stack<Node*>  keller;
		Node         *root_node;
		ParserState   state;
		const char   *band;
		const char   *band_position;
		bool          enable_colors;
		bool          only_colors;
		std::string   last_error;

		/* Decide what state should be used to parse the current type of token. */
		ParserState state_START();
		/* Parse string, color and property. */
		ParserState state_TEXT();
		ParserState state_COLOR();
		ParserState state_PROPERTY();
		/* Parse if-else-fi constructs. */
		ParserState state_IF();
		ParserState state_ELSE();
		ParserState state_FI();

	public:
		FormatParser() : root_node(NULLPTR) {
		}

		/* There is no destructor: root_node will not be deleted! */
#if defined(EIX_PARANOIC_ASSERT)
		~FormatParser() {
			eix_assert_paranoic(keller.empty());
		}
#endif

		ATTRIBUTE_NONNULL((2)) bool start(const char *fmt, bool colors, bool parse_only_colors, std::string *errtext);

		Node *rootnode() {
			return root_node;
		}

		/**
		Calculate line and column of current position
		**/
		void getPosition(size_t *line, size_t *column);
};

class VarParserCacheNode {
	private:
		Node *root_node;

	public:
		bool in_use;

		VarParserCacheNode() : root_node(NULLPTR) {
		}

		~VarParserCacheNode() {
			delete root_node;
		}

		ATTRIBUTE_NONNULL((2)) bool init(const char *fmt, bool colors, bool use, std::string *errtext);
		Node *rootnode() {
			return root_node;
		}
};

typedef UNORDERED_MAP<std::string, VarParserCacheNode> VarParserCacheMap;

class VarParserCache : public VarParserCacheMap {
	public:
		void clear_use();
};

class VersionVariables;

class PrintFormat {
	friend class LocalCopy;
	friend class Scanner;
	ATTRIBUTE_NONNULL_ friend void get_package_property(OutputString *s, const PrintFormat *fmt, void *entity, const std::string& name);
	ATTRIBUTE_NONNULL_ friend void get_diff_package_property(OutputString *s, const PrintFormat *fmt, void *void_entity, const std::string& name);

	public:
		ATTRIBUTE_NONNULL_ typedef void (*GetProperty)(OutputString *s, const PrintFormat *fmt, void *entity, const std::string& property);
		typedef std::vector<ExtendedVersion::Overlay> OverlayTranslations;
		typedef std::vector<bool> OverlayUsed;

	protected:
		typedef std::vector<Version*> VerVec;
		enum HandleExpand { EXPAND_NO, EXPAND_YES, EXPAND_OMIT };

		static std::string::size_type currcolumn;

		mutable UNORDERED_MAP<std::string, OutputString> user_variables;
		/* Looping over variables is a bit tricky:
		   We store the parsed thing in VarParserCache.
		   Additionally, we store there whether we currently loop
		   over the variable to avoid recursion. */
		mutable VarParserCache varcache;
		mutable VersionVariables *version_variables;
		Node          *root_node;
		GetProperty    m_get_property;
		typedef std::vector<bool> Virtuals;
		Virtuals *virtuals;
		OverlayTranslations *overlay_translations;
		OverlayUsed *overlay_used;
		bool          *some_overlay_used;
		MaskList<Mask> *marked_list;
		EixRc         *eix_rc;
		/* The following four variables are actually a hack:
		   This is only set temporarily during printing to avoid
		   passing this argument through all sub-functions.
		   We do it that way since we do not want to set it "globally":
		   The pointers might possibly have changed until we use them */
		const DBHeader *header;
		VarDbPkg       *vardb;
		const PortageSettings *portagesettings;
		const SetStability *stability;

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL((3)) bool recPrint(OutputString *result, void *entity, GetProperty get_property, Node *root) const;

		/* return true if something was actually printed */
		bool printString(OutputString *result, const OutputString& output) const;

		ATTRIBUTE_NONNULL((2)) bool parse_variable(Node **rootnode, const std::string& varname, std::string *errtext) const;
		Node *parse_variable(const std::string& varname) const;

		void iuse_expand(OutputString *s, const IUseSet& iuse, bool coll, HandleExpand expand) const;
		ATTRIBUTE_NONNULL_ void get_inst_use(OutputString *s, const Package& package, InstVersion *i, HandleExpand expand) const;
		ATTRIBUTE_NONNULL((2)) void get_installed(Package *package, Node *root) const;
		ATTRIBUTE_NONNULL((2)) void get_versions_versorted(Package *package, Node *root, PrintFormat::VerVec *versions) const;
		ATTRIBUTE_NONNULL((2)) void get_versions_slotsorted(Package *package, Node *root, PrintFormat::VerVec *versions) const;
		ATTRIBUTE_NONNULL_ void get_pkg_property(OutputString *s, Package *package, const std::string& name) const;

		// It follows a list of indirect functions called in get_pkg_property():
		// Functions with capital letters are parser destinations; other functions
		// here are sort of "macros" used by several other "capital letter" functions.

		ATTRIBUTE_NONNULL_ void COLON_VER_DATE(OutputString *s, Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void colon_pkg_availableversions(Package *package, const std::string& after_colon, bool only_marked) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_AVAILABLEVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_MARKEDVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void colon_pkg_bestversion(Package *package, const std::string& after_colon, bool allow_unstable) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTVERSION(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void colon_pkg_bestslotversions(Package *package, const std::string& after_colon, bool allow_unstable) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTSLOTVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTSLOTVERSIONSS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void colon_pkg_bestslotupgradeversions(Package *package, const std::string& after_colon, bool allow_unstable) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTSLOTUPGRADEVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_BESTSLOTUPGRADEVERSIONSS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void COLON_PKG_INSTALLEDVERSIONS(Package *package, const std::string& after_colon) const;
		ATTRIBUTE_NONNULL_ void PKG_INSTALLED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_VERSIONLINES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_SLOTSORTED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_COLOR(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVEBEST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVEBESTS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_CATEGORY(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_NAME(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_DESCRIPTION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HOMEPAGE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_LICENSES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BINARY(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_MAINREPO(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_OVERLAYKEY(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_OVERLAYNAME(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_SYSTEM(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_PROFILE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_WORLD(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_WORLD_SETS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_SETNAMES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_ALLSETNAMES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void pkg_upgrade(OutputString *s, Package *package, bool only_installed, bool test_slots) const;
		ATTRIBUTE_NONNULL_ void PKG_UPGRADE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_UPGRADEORINSTALL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BESTUPGRADE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BESTUPGRADEORINSTALL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void pkg_downgrade(OutputString *s, Package *package, bool test_slots) const;
		ATTRIBUTE_NONNULL_ void PKG_DOWNGRADE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BESTDOWNGRADE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void pkg_recommend(OutputString *s, Package *package, bool only_installed, bool test_slots) const;
		ATTRIBUTE_NONNULL_ void PKG_RECOMMEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_RECOMMENDORINSTALL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BESTRECOMMEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_BESTRECOMMENDORINSTALL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_MARKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVEMARKEDVERSION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_SLOTS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_SLOTTED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVEVIRTUAL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVENONVIRTUAL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_HAVECOLLIUSE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_COLLIUSE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_COLLIUSES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void PKG_COLLIUSE0(OutputString *s, Package *package) const;
		ATTRIBUTE_PURE const ExtendedVersion *ver_version() const;
		ATTRIBUTE_NONNULL_ void VER_FIRST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_LAST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_SLOTFIRST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_SLOTLAST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ONESLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ const ExtendedVersion *ver_versionslot(Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_FULLSLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISFULLSLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_SLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISSLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_SUBSLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISSUBSLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVESRCURI(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_SRCURI(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_EAPI(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_VERSION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PLAINVERSION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_REVISION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void ver_overlay(OutputString *s, Package *package, bool numeric, bool only_noncommon, bool only_nonzero) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYNUM(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYVER(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYPLAINNAMES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYPLAINNAME(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYVERNAMES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_OVERLAYVERNAME(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_VERSIONKEYWORDSS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_VERSIONKEYWORDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_VERSIONEKEYWORDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void ver_isbestupgrade(OutputString *s, Package *package, bool check_slots, bool allow_unstable) const;
		ATTRIBUTE_NONNULL_ void VER_ISBESTUPGRADESLOT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISBESTUPGRADESLOTS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISBESTUPGRADE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISBESTUPGRADES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_MARKEDVERSION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_INSTALLEDVERSION(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEUSE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_USE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_USES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_USE0(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_REQUIREDUSE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEREQUIREDUSE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_VIRTUAL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISBINARY(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISTBZ(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISGPKG(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISPAK(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMULTIGPKG(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMULTIPAK(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_GPKGCOUNT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PAKCOUNT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ const ExtendedVersion *ver_restrict(Package *package) const;
		ATTRIBUTE_NONNULL_ void ver_restrict(OutputString *s, Package *package, ExtendedVersion::Restrict r) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICT(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTFETCH(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTMIRROR(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTPRIMARYURI(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTBINCHECKS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTSTRIP(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTTEST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTUSERPRIV(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTINSTALLSOURCES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTBINDIST(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RESTRICTPARALLEL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void ver_properties(OutputString *s, Package *package, ExtendedVersion::Properties p) const;
		ATTRIBUTE_NONNULL_ void VER_PROPERTIES(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PROPERTIESINTERACTIVE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PROPERTIESLIVE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PROPERTIESVIRTUAL(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PROPERTIESSET(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVERDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEPDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEBDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEIDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEDEPS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_DEPENDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_DEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RDEPENDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_RDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PDEPENDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_PDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_BDEPENDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_BDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_IDEPENDS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_IDEPEND(OutputString *s, Package *package) const;
		ATTRIBUTE_PURE const MaskFlags *ver_maskflags() const;
		ATTRIBUTE_NONNULL_ void VER_ISHARDMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISPROFILEMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_PURE const KeywordsFlags *ver_keywordsflags() const;
		ATTRIBUTE_NONNULL_ void VER_ISSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISALIENSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISALIENUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMISSINGKEYWORD(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMINUSKEYWORD(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMINUSUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_ISMINUSASTERISK(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL((2)) bool ver_wasflags(Package *package, MaskFlags *maskflags, KeywordsFlags *keyflags) const;
		ATTRIBUTE_NONNULL_ void VER_WASHARDMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASPROFILEMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASMASKED(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASALIENSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASALIENUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASMISSINGKEYWORD(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASMINUSKEYWORD(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASMINUSUNSTABLE(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_WASMINUSASTERISK(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_HAVEMASKREASONS(OutputString *s, Package *package) const;
		void ver_maskreasons(OutputString *s, const OutputString& skip, const OutputString& sep) const;
		ATTRIBUTE_NONNULL_ void VER_MASKREASONS(OutputString *s, Package *package) const;
		ATTRIBUTE_NONNULL_ void VER_MASKREASONSS(OutputString *s, Package *package) const;

	public:
		bool	no_color,             ///< Shall we use colors?
			style_version_lines,  ///< Shall we show versions linewise?
			slot_sorted,          ///< Print sorted by slots
			alpha_use;            ///< Print use in alphabetical order (not by set/unset)

		LocalMode recommend_mode;

		std::string
			color_overlaykey,  ///< Color for the overlay key
			color_virtualkey,  ///< Color for the virtual key
			color_keyend,
			color_overlayname, color_overlaynameend,
			color_numbertext, color_numbertextend, color_end;
		OutputString
			before_use_start, before_use_end, after_use,
			before_iuse_start, before_iuse_end, after_iuse,
			before_coll_start, before_coll_end, after_coll,
			before_set_use, after_set_use,
			before_unset_use, after_unset_use,
			maskreasons_skip,  maskreasons_sep,
			maskreasonss_skip, maskreasonss_sep;

		void init(GetProperty get_callback);

		PrintFormat() {
			init(NULLPTR);
		}

		explicit PrintFormat(GetProperty get_callback) {
			init(get_callback);
		}

		~PrintFormat() {
			delete root_node;
		}

		// Initialize those variables common to eix and eix-diff:
		ATTRIBUTE_NONNULL_ void setupResources(EixRc *eixrc);

		void setupColors();

		void clear_virtual(ExtendedVersion::Overlay count);

		void set_as_virtual(const ExtendedVersion::Overlay overlay, bool on);
		void set_as_virtual(const ExtendedVersion::Overlay overlay) {
			set_as_virtual(overlay, true);
		}

		ATTRIBUTE_PURE bool is_virtual(const ExtendedVersion::Overlay overlay) const;

		ATTRIBUTE_NONNULL_ ATTRIBUTE_PURE bool have_virtual(const Package *p, bool nonvirtual) const;

		void set_overlay_translations(OverlayTranslations *translations) {
			overlay_translations = translations;
		}

		void set_overlay_used(OverlayUsed *used, bool *some) {
			overlay_used = used;
			some_overlay_used = some;
		}

		void set_marked_list(MaskList<Mask> *m_list) {
			marked_list = m_list;
		}

		ATTRIBUTE_NONNULL_ void overlay_keytext(OutputString *s, ExtendedVersion::Overlay overlay, bool plain) const;
		ATTRIBUTE_NONNULL_ void overlay_keytext(OutputString *s, ExtendedVersion::Overlay overlay) const {
			overlay_keytext(s, overlay, false);
		}

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL((2, 5, 6, 7, 8)) bool print(void *entity, GetProperty get_property, Node *root, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s, bool check_only);

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL((2, 5, 6, 7, 8)) bool print(void *entity, GetProperty get_property, Node *root, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s) {
			return print(entity, get_property, root, dbheader, vardbpkg, ps, s, false);
		}

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL((2, 4, 5, 6, 7)) bool print(void *entity, Node *root, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s, bool check_only) {
			return print(entity, m_get_property, root, dbheader, vardbpkg, ps, s, check_only);
		}

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL((2, 4, 5, 6, 7)) bool print(void *entity, Node *root, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s) {
			return print(entity, m_get_property, root, dbheader, vardbpkg, ps, s);
		}

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL_ bool print(void *entity, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s, bool check_only) {
			return print(entity, root_node, dbheader, vardbpkg, ps, s, check_only);
		}

		/* return true if something was actually printed */
		ATTRIBUTE_NONNULL_ bool print(void *entity, const DBHeader *dbheader, VarDbPkg *vardbpkg, const PortageSettings *ps, const SetStability *s) {
			return print(entity, root_node, dbheader, vardbpkg, ps, s);
		}

		ATTRIBUTE_NONNULL((2, 3)) bool parseFormat(Node **rootnode, const char *fmt, std::string *errtext);

		ATTRIBUTE_NONNULL((2)) bool parseFormat(const char *fmt, std::string *errtext) {
			delete root_node;
			return parseFormat(&root_node, fmt, errtext);
		}

		ATTRIBUTE_NONNULL_ void StabilityLocal(Package *p) const {
			stability->set_stability(true, p);
		}

		ATTRIBUTE_NONNULL_ void StabilityNonlocal(Package *p) const {
			stability->set_stability(false, p);
		}

		static void init_static();
};

class LocalCopy : public PackageSave {
	public:
		ATTRIBUTE_NONNULL_ LocalCopy(const PrintFormat *fmt, Package *pkg);
};



#endif  // SRC_OUTPUT_FORMATSTRING_H_
