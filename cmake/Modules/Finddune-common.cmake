# - Find DUNE common library
#
# Defines the following variables:
#   dune-common_INCLUDE_DIRS    Directory of header files
#   dune-common_LIBRARIES       Directory of shared object files
#   dune-common_DEFINITIONS     Defines that must be set to compile
#   dune-common_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_COMMON            Binary value to use in config.h

# Copyright (C) 2012 Uni Research AS
# This code is licensed under The GNU General Public License v3.0

include (OpmPackage)
find_opm_package (
  # module name
  "dune-common"

  # dependencies
  "BLAS REQUIRED;
  LAPACK REQUIRED;
  CxaDemangle;
  MPI;
  TBB
  "
  # header to search for
  "dune/common/fvector.hh"

  # library to search for
  "dunecommon"

  # defines to be added to compilations
  ""

  # test program
"#include <dune/common/fvector.hh>
#include <dune/common/fmatrix.hh>
int main (void) {
  Dune::FieldVector<double,1> v;
  Dune::FieldMatrix<double,1,1> m;
  m[0][0] = 1.0;
  v[0]    = 1.0;
  Dune::FieldVector<double,1> w = m*v;
  return 0;
}
"
  # config variables
  "HAS_ATTRIBUTE_UNUSED;
  HAS_ATTRIBUTE_DEPRECATED;
  HAS_ATTRIBUTE_DEPRECATED_MSG;
  HAVE_ARRAY;
  HAVE_BOOST_MAKE_SHARED_HPP;
  HAVE_BOOST_SHARED_PTR_HPP;
  HAVE_DUNE_BOOST;
  HAVE_GMP;
  HAVE_MAKE_SHARED;
  HAVE_MPI;
  HAVE_NULLPTR;
  HAVE_STATIC_ASSERT;
  HAVE_SHARED_PTR;
  MPI_2;
  SHARED_PTR_HEADER;
  SHARED_PTR_NAMESPACE;
  HAVE_TYPE_TRAITS;
  HAVE_TR1_TUPLE;
  HAVE_TUPLE;
  HAVE_CXA_DEMANGLE
  ")
#debug_find_vars ("dune-common")

if(MPI_C_FOUND)
  # check for MPI version 2
  include(CMakePushCheckState)
  include(CheckFunctionExists)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES};${MPI_C_LIBRARIES})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES};${MPI_C_INCLUDES})
  check_function_exists(MPI_Finalized MPI_2)
  cmake_pop_check_state()
endif(MPI_C_FOUND)

# make version number available in config.h
include (UseDuneVer)
find_dune_version ("dune" "common")
