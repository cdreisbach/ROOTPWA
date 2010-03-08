///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2010
//
//    This file is part of rootpwa
//
//    rootpwa is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    rootpwa is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with rootpwa. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------
// File and Version Information:
// $Rev::                             $: revision of last commit
// $Author::                          $: author of last commit
// $Date::                            $: date of last commit
//
// Description:
//      container class for particle properties
//
//
// Author List:
//      Boris Grube          TUM            (original author)
//
//
//-------------------------------------------------------------------------


#include "utilities.h"
#include "particleDataTable.h"
#include "particleProperties.h"

	
using namespace std;
using namespace rpwa;


bool particleProperties::_debug = false;

	
particleProperties::particleProperties()
  : _name     (""),
    _mass     (0),
    _width    (0),
    _baryonNmb(0),
    _I        (0),
    _S        (0),
    _Charm    (0),
    _B        (0),
    _G        (0),
    _J        (0),
    _P        (0),
    _C        (0)
{ }


particleProperties::particleProperties(const particleProperties& partProp)
{
  *this = partProp;
}



particleProperties::~particleProperties()
{ }


particleProperties&
particleProperties::operator = (const particleProperties& partProp)
{
  if (this != &partProp) {
    _name      = partProp._name;
    _mass      = partProp._mass;
    _width     = partProp._width;
    _baryonNmb = partProp._baryonNmb;
    _I         = partProp._I;
    _S         = partProp._S;
    _Charm     = partProp._Charm;
    _B         = partProp._B;
    _G         = partProp._G;
    _J         = partProp._J;
    _P         = partProp._P;
    _C         = partProp._C;
  }
  return *this;
}


bool
operator == (const particleProperties& lhsProp,
	     const particleProperties& rhsProp)
{
  return (   (lhsProp.name()      == rhsProp.name()     )
          && (lhsProp.mass()      == rhsProp.mass()     )
          && (lhsProp.width()     == rhsProp.width()    )
          && (lhsProp.baryonNmb() == rhsProp.baryonNmb())
          && (lhsProp.I()         == rhsProp.I()        )
          && (lhsProp.S()         == rhsProp.S()        )
          && (lhsProp.Charm()     == rhsProp.Charm()    )
          && (lhsProp.B()         == rhsProp.B()        )
          && (lhsProp.G()         == rhsProp.G()        )
          && (lhsProp.J()         == rhsProp.J()        )
          && (lhsProp.P()         == rhsProp.P()        )
	  && (lhsProp.C()         == rhsProp.C()        ));
}


bool
particleProperties::fillFromDataTable(const string& name)
{
  const particleProperties* partProp = particleDataTable::instance().entry(name);
  if (!partProp) {
    printWarn << "trying to fill particle properties for '" << name << "' from non-existing table entry" << endl;
    return false;
  } else {
    *this = *partProp;
    return true;
  }
}


void
particleProperties::print(ostream& out) const
{
  out << "particle '"  << _name         << "': "
      << "mass = "     << _mass         << ", "
      << "width = "    << _width        << ", "
      << "baryon # = " << _baryonNmb    << ", "
      << "IG(JPC)"     << _I << sign(_G) << "(" << _J << sign(_P) << sign(_C) << "),  "
      << "S = "        << _S            << ", "
      << "Charm = "    << _Charm        << ", "
      << "B = "        << _B
      << endl;
}


void
particleProperties::dump(ostream& out) const
{
  out << _name      << "\t"
      << _mass      << "\t"
      << _width     << "\t"
      << _baryonNmb << "\t"
      << _I         << "\t"
      << _S         << "\t" 
      << _Charm     << "\t" 
      << _B         << "\t" 
      << _G         << "\t" 
      << _J         << "\t" 
      << _P         << "\t" 
      << _C         << endl;
}


ostream&
operator << (ostream&                  out,
	     const particleProperties& partProp)
{
  partProp.print(out);
  return out;
}


bool
particleProperties::read(istringstream& line)
{
  if (_debug)
    printInfo << "trying to read particle properties from line '" << line << "' ... " << flush;
  if (line >> _name
           >> _mass
           >> _width
           >> _baryonNmb
           >> _I
           >> _S
           >> _Charm
           >> _B
           >> _G
           >> _J
           >> _P
           >> _C) {
    if (_debug)
      cout << "success" << endl
	   << *this     << endl;
    return true;
  } else {
    printWarn << "problems reading particle data from line '" << line.str() << "'" << endl;
    return false;
  }
}


istream&
operator >> (istream&            in,
	     particleProperties& partProp)
{
  string line;
  if (getline(in, line)) {
    // skip comments and empty lines
    while ((line == "") || (line[0] == '#'))
      if (partProp.debug())
	printInfo << "ignoring line '" << line << "'" << endl;
      if (!getline(in, line)) {
	printWarn << "could not find valid particle entry before end of file" << endl;
	return in;
      }
    istringstream lineStream(line);
    partProp.read(lineStream);
  }
  return in;
}
