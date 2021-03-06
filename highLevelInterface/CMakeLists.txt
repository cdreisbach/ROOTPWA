#///////////////////////////////////////////////////////////////////////////
#//
#//    Copyright 2010
#//
#//    This file is part of rootpwa
#//
#//    rootpwa is free software: you can redistribute it and/or modify
#//    it under the terms of the GNU General Public License as published by
#//    the Free Software Foundation, either version 3 of the License, or
#//    (at your option) any later version.
#//
#//    rootpwa is distributed in the hope that it will be useful,
#//    but WITHOUT ANY WARRANTY; without even the implied warranty of
#//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//    GNU General Public License for more details.
#//
#//    You should have received a copy of the GNU General Public License
#//    along with rootpwa.  If not, see <http://www.gnu.org/licenses/>.
#//
#///////////////////////////////////////////////////////////////////////////
#//-------------------------------------------------------------------------
#//
#// Description:
#//      build file for amplitude library
#//
#//
#// Author List:
#//      Boris Grube          TUM            (original author)
#//
#//
#//-------------------------------------------------------------------------


message_setup_this_dir()


# set include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${RPWA_DECAYAMPLITUDE_INCLUDE_DIR}
	${RPWA_PARTIALWAVEFIT_INCLUDE_DIR}
	${RPWA_PARTICLEDATA_INCLUDE_DIR}
	${RPWA_STORAGEFORMATS_INCLUDE_DIR}
	${RPWA_UTILITIES_INCLUDE_DIR}
	SYSTEM
	${Boost_INCLUDE_DIRS}
	${Libconfig_INCLUDE_DIR}
	${PYTHON_INCLUDE_DIRS}
	${ROOT_INCLUDE_DIR}
	)
if(USE_NLOPT)
	include_directories(
		SYSTEM
		${NLopt_INCLUDE_DIR}
	)
endif()


# source files that are compiled into library
set(SOURCES
	calcAmplitude.cc
	pwaFit.cc
	getMassShapes.cc
	)
if(USE_NLOPT)
	LIST(APPEND SOURCES pwaNloptFit.cc)
endif()


# library name
lib_name_for_current_dir(THIS_LIB)


# library
make_shared_library(
	"${THIS_LIB}"
	"${SOURCES}"
	"${Libconfig_LIBS}"
	"${ROOT_LIBS}"
	"${RPWA_DECAYAMPLITUDE_LIB}"
	"${RPWA_PARTIALWAVEFIT_LIB}"
	"${RPWA_PARTICLEDATA_LIB}"
	"${RPWA_STORAGEFORMATS_LIB}"
	)
if(USE_NLOPT)
	target_link_libraries(${THIS_LIB} "${NLopt_LIBS}")
endif()
