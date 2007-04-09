/*
 * Copyright (C) 2003 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Class which parse an config file
// C++ warper for the iniparser free library from N.Devillard

#include <sstream>
#include <iomanip>

#include "Config.h"

Config::Config(void) : dico(NULL)
{
	dico = dictionary_new(0);
}

Config::~Config()
{
	if (dico) free_dico();
	dico = NULL;
}

void Config::load(const string& file)
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(file.c_str(),"rt");
	if (fp)
	{
		fclose(fp);
	}
	else
	{
		debug("ERROR : Can't find config file %s\n", file.c_str());
		//exit(-1);
	}

	if (dico) free_dico();
	dico = NULL;
	dico = iniparser_load(file.c_str());
	if (!dico)
	{
		debug("ERROR : Couldn't read config file %s\n", file.c_str());
		//exit(-1);
	}
}

void Config::save(const string& file_name) const
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(file_name.c_str(),"wt");
	if (fp)
	{
		iniparser_dump_ini(dico, fp);
	}
	else
	{
		debug("ERROR : Can't open file %s\n", file_name.c_str());
		//exit(-1);
	}
	if (fp) fclose(fp);
}

string Config::get_str(const string& key) const
{
	if (iniparser_getstring(dico, key.c_str(), NULL)) return string(iniparser_getstring(dico, key.c_str(), NULL));
	else debug("WARNING : Can't find the configuration key \"%s\", default empty string returned", key.c_str());
	return string();
}

string Config::get_str(const string& section, const string& key) const
{
	return get_str((section+":"+key).c_str());
}

string Config::get_str(const string& section, const string& key, const string& def) const
{
	return iniparser_getstring(dico, (section+":"+key).c_str(), def.c_str());
}

int Config::get_int(const string& key) const
{
	int i = iniparser_getint(dico, key.c_str(), -11111);
	// To be sure :) (bugfree)
	if (i==-11111)
	{
		i = iniparser_getint(dico, key.c_str(), 0);
		if (i==0)
		{
			debug("WARNING : Can't find the configuration key \"%s\", default 0 value returned\n", key.c_str());
		}
	}
	return i;
}

int Config::get_int(const string& section, const string& key) const
{
	return get_int((section+":"+key).c_str());
}

int Config::get_int(const string& section, const string& key, int def) const
{
	return iniparser_getint(dico, (section+":"+key).c_str(), def);
}

double Config::get_double(const string& key) const
{
	double d = iniparser_getdouble(dico, key.c_str(), -11111.111111356);
	// To be sure :) (bugfree)
	if (d==-11111.111111356)
	{
		d = iniparser_getdouble(dico, key.c_str(), 0.);
		if (d==0.)
		{
			debug("WARNING : Can't find the configuration key \"%s\", default 0 value returned\n", key.c_str());
		}
	}
	return d;
}

double Config::get_double(const string& section, const string& key) const
{
	return get_double((section+":"+key).c_str());
}

double Config::get_double(const string& section, const string& key, double def) const
{
	return iniparser_getdouble(dico, (section+":"+key).c_str(), def);
}

bool Config::get_boolean(const string& key) const
{
	int b = iniparser_getboolean(dico, key.c_str(), -10);
	// To be sure :) (bugfree)
	if (b==-10)
	{
		b = iniparser_getboolean(dico, key.c_str(), 0);
		if (b==0)
		{
			debug("WARNING : Can't find the configuration key \"%s\", default 0 value returned\n", key.c_str());
		}
	}
	return b;
}

bool Config::get_boolean(const string& section, const string& key) const
{
	return get_boolean((section+":"+key).c_str());
}

bool Config::get_boolean(const string& section, const string& key, bool def) const
{
	return iniparser_getboolean(dico, (section+":"+key).c_str(), def);
}

// Set the given entry with the provided value. If the entry cannot be found
// -1 is returned and the entry is created. Else 0 is returned.
int Config::set_str(const string& key, const string& val)
{
	make_section_from_key(key);
	int return_val;
	if (find_entry(key)) return_val = 0;
	else return_val = -1;

	dictionary_set(dico, key.c_str(), val.c_str());
	return return_val;
}

int Config::set_int(const string& key, int val)
{
	make_section_from_key(key);
	int return_val;
	if (find_entry(key)) return_val = 0;
	else return_val = -1;

	dictionary_setint(dico, key.c_str(), val);

	return return_val;
}

int Config::set_double(const string& key, double val)
{
	make_section_from_key(key);
	int return_val;
	if (find_entry(key)) return_val = 0;
	else return_val = -1;

	ostringstream os;
	os << setprecision(16) << val;
	dictionary_set(dico, key.c_str(), os.str().c_str());

	return return_val;
}

int Config::set_boolean(const string& key, bool val)
{
	if (val) return set_str(key, "true");
	else return set_str(key, "false");
}

// Check if the key is in the form section:key and if yes create the section in the dictionnary
// if it doesn't exist.
void Config::make_section_from_key(const string& key)
{
	int pos = key.find(':');
	if (!pos) return;				// No ':' were found
	string sec = key.substr(0,pos);
	if (find_entry(sec)) return;	// The section is already present into the dictionnary
	dictionary_set(dico, sec.c_str(), NULL);	// Add the section key
}

// Get number of sections
int Config::get_nsec(void) const
{
	return iniparser_getnsec(dico);
}

// Get name for section n.
string Config::get_secname(int n) const
{
	if (iniparser_getsecname(dico, n)) return string(iniparser_getsecname(dico, n));
	return string();
}

// Return 1 if the entry exists, 0 otherwise
int Config::find_entry(const string& entry) const
{
	return iniparser_find_entry(dico, entry.c_str());
}

void Config::free_dico(void)
{
	iniparser_freedict(dico);
}
