// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)                                                         
//   Wolfgang Frisch <xororand@users.sourceforge.net>                    
//   Emil Beinroth <emilbeinroth@gmx.net>                                

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <iostream>
#include <string>
#include <vector>

#include <errno.h>
#include <stdarg.h>

#include <eixTk/stringutils.h>

/** The exception for everything. */
class ExBasic {

	public:

		/** Constructor exception with variable arguments. */
		ExBasic(const std::string file, const int line, const char *func, const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			char buf[1025];
			vsnprintf(buf, 1024, fmt, ap);
			va_end(ap);

			m_msg  = buf;
			m_file = file;
			m_line = line;
			m_func = func;
		}

		const std::string &getMessage() const throw()
		{ return m_msg; }

		const char *what() const
		{ return m_msg.c_str(); }

		friend std::ostream& operator<< (std::ostream& os, ExBasic& e)
		{ return os << e.m_func << ": " << e.m_msg; }

	protected:
		std::string m_file; /**< File where the exception is constructed. */
		int    m_line; /**< Line where the exception is constructed. */
		std::string m_func; /**< Function where the exception is constructed. */
		std::string m_msg;  /**< The actual message. */

	private:
};

#define ExBasic(...) ExBasic(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#define THROW(...) throw(ExBasic(__VA_ARGS__))
#define OOM_ASSERT(x) ASSERT((x), "Out of memory.")

#define ASSERT(x, ...) do { \
	if(!(x)) throw( ExBasic("assert("#x"): " __VA_ARGS__) ); \
} while(0)

#define WARNING(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while(0)

// Provide a common look for error-messages for parse-errors in
// portage.{mask,keywords,..}
inline void
portage_parse_error(const std::string &file, const int line_nr, const std::string& line, const ExBasic &e)
{
	std::cerr << "-- Invalid line in "<< file << "("<< line_nr <<"): \""
	     << line << "\"" << std::endl;

	// Indent the message correctly
	std::vector<std::string> lines = split_string(e.getMessage(), "\n", false);
	for(std::vector<std::string>::iterator i = lines.begin();
		i != lines.end();
		++i)
	{
		std::cerr << "    " << *i << std::endl;
	}
	std::cerr << std::endl;
}

#endif /* __EXCEPTIONS_H__ */
