/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "guilanguage.h"
#include "cfg/config.h"
#include <iconv.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#ifndef POSIX
// inbuf not const due to iconv interface (posix wtf!?)
size_t iconv(iconv_t cd, char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft)
{
	return iconv(cd, (const char**)inbuf, inbytesleft, outbuf, outbytesleft);
}
#endif

// language to codepage mapping
static const std::map<std::string, std::string> InitCP();
static const std::map<std::string, std::string> codepages(InitCP());
static const std::string default_codepage("1252");

// convert input string using given conversion descriptor
// the returned string has to be freed manually
static char * convert(iconv_t cd, char *input);


GUILANGUAGE::GUILANGUAGE() :
	m_lang_id("en"),
	m_iconv(0)
{
	//ctor
}

GUILANGUAGE::~GUILANGUAGE()
{
	if (m_iconv)
		iconv_close(m_iconv);
}

void GUILANGUAGE::Init(const std::string & lang_path)
{
	m_lang_path = lang_path;
}

void GUILANGUAGE::Set(const std::string & lang_id)
{
	if (m_lang_id != lang_id)
	{
		m_lang_id = lang_id;
		LoadLanguage();
	}
}

const std::string & GUILANGUAGE::GetCodePageId(const std::string & lang_id)
{
	std::map<std::string, std::string>::const_iterator i = codepages.find(lang_id);
	if (i != codepages.end())
		return i->second;
	return default_codepage;
}

void GUILANGUAGE::LoadLanguage()
{
	Config cfg(m_lang_path + "/" + m_lang_id + ".txt");
	Config::const_iterator i;
	if (!cfg.get("", i))
		return;

	const std::string to = "CP" + GetCodePageId(m_lang_id);
	if (m_iconv)
		iconv_close(m_iconv);
	m_iconv = iconv_open(to.c_str(), "UTF-8");
	if (m_iconv == iconv_t(-1))
	{
		assert(0);
		return;
	}

	m_strings.clear();
	const Config::Section & sn = i->second;
	for (Config::Section::const_iterator n = sn.begin(); n != sn.end(); ++n)
	{
		char * str = strdup(n->second.c_str());
		char * cstr = convert(m_iconv, str);
		if (cstr)
		{
			m_strings[n->first] = std::string(cstr);
			free(cstr);
		}
		free(str);
	}
}

const std::map<std::string, std::string> InitCP()
{
	std::map<std::string, std::string> cp;
	cp["ar"] = "1256";
	cp["bg"] = "1251";
	cp["ca"] = "1252";
	cp["cs"] = "1250";
	cp["da"] = "1252";
	cp["de"] = "1252";
	cp["el"] = "1253";
	cp["en"] = "1252";
	cp["es"] = "1252";
	cp["et"] = "1257";
	cp["fi"] = "1252";
	cp["fr"] = "1252";
	cp["hr"] = "1250";
	cp["hu"] = "1250";
	cp["is"] = "1252";
	cp["it"] = "1252";
	cp["iw"] = "1255";
	cp["ja"] = "932";
	cp["ko"] = "949";
	cp["lt"] = "1257";
	cp["lv"] = "1257";
	cp["mk"] = "1251";
	cp["nl"] = "1252";
	cp["no"] = "1252";
	cp["pl"] = "1250";
	cp["pt"] = "1252";
	cp["ro"] = "1250";
	cp["ru"] = "1251";
	cp["sh"] = "1250";
	cp["sk"] = "1250";
	cp["sl"] = "1250";
	cp["sq"] = "1252";
	cp["sr"] = "1251";
	cp["sv"] = "1252";
	cp["th"] = "874";
	cp["tr"] = "1254";
	cp["zh"] = "936";
	return cp;
}

char * convert(iconv_t cd, char *input)
{
	size_t inleft, outleft, converted = 0;
	char *output, *outbuf, *tmp;
	char *inbuf;
	size_t outlen;

	inleft = strlen(input);
	inbuf = input;

	/* start with an output buffer of the same size as input buffer */
	outlen = inleft;

	/* allocate 4 bytes more than what needed for nul-termination... */
	output = (char*)malloc(outlen + 4);
	if (!output)
		return NULL;

	while(1)
	{
		errno = 0;
		outbuf = output + converted;
		outleft = outlen - converted;

		converted = iconv(cd, &inbuf, &inleft, &outbuf, &outleft);
		if (converted != (size_t) -1 || errno == EINVAL)
		{
			/*
			* EINVAL  An  incomplete  multibyte sequence has been encoun­-
			*         tered in the input.
			*
			* Just truncate and ignore it.
			*/
			break;
		}

		if (errno != E2BIG)
		{
			/*
			* EILSEQ An invalid multibyte sequence has been  encountered
			*        in the input.
			*
			* Bad input, can't really recover from this.
			*/
			free(output);
			return NULL;
		}

		/*
		* E2BIG   There is not sufficient room at *outbuf.
		*
		* Need to grow outbuffer and try again.
		*/
		converted = outbuf - output;
		outlen += inleft * 2 + 8;
		tmp = (char*)realloc(output, outlen + 4);
		if (!tmp)
		{
			free(output);
			return NULL;
		}

		output = tmp;
		outbuf = output + converted;
	}

	/* flush the iconv conversion */
	iconv(cd, (char**)NULL, (size_t*)NULL, &outbuf, &outleft);

	/* Note: not all charsets can be nul-terminated with a single
	* nul byte. UCS2, for example, needs 2 nul bytes and UCS4
	* needs 4. I hope that 4 nul bytes is enough to terminate all
	* multibyte charsets? */

	/* null-terminate the string */
	memset(outbuf, 0, 4);

	return output;
}

