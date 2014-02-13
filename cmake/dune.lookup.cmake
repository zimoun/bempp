# Needs boost: include this after boost.lookup.
if(USE_OWN_dune)
  find_package(dune 2.3.0 COMPONENTS geometry grid localfunctions foamgrid)
endif()
if(NOT DUNE_FOUND)
  ExternalProject_Add(
      dune-common
      PREFIX ${EXTERNAL_ROOT}
      URL http://www.dune-project.org/download/2.3.0/dune-common-2.3.0.tar.gz
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_ROOT}
                 -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE
                 -DCMAKE_PROGRAM_PATH:PATH=${EXTERNAL_ROOT}/bin
                 -DCMAKE_LIBRARY_PATH:PATH=${EXTERNAL_ROOT}/lib
                 -DCMAKE_INCLUDE_PATH:PATH=${EXTERNAL_ROOT}/include
                 -DDUNE_USE_ONLY_STATIC_LIBS=TRUE
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
  )

  ExternalProject_Add(
      dune-geometry
      DEPENDS dune-common
      PREFIX ${EXTERNAL_ROOT}
      URL http://www.dune-project.org/download/2.3.0/dune-geometry-2.3.0.tar.gz
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_ROOT}
                  -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE
                 -DCMAKE_PROGRAM_PATH:PATH=${EXTERNAL_ROOT}/bin
                 -DCMAKE_LIBRARY_PATH:PATH=${EXTERNAL_ROOT}/lib
                 -DCMAKE_INCLUDE_PATH:PATH=${EXTERNAL_ROOT}/include
                 -DDUNE_USE_ONLY_STATIC_LIBS=TRUE
                 -Ddune-common_DIR=${EXTERNAL_ROOT}/src/dune-common-build
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
  )

  ExternalProject_Add(
      dune-grid
      DEPENDS dune-geometry dune-common
      PREFIX ${EXTERNAL_ROOT}
      URL http://www.dune-project.org/download/2.3.0/dune-grid-2.3.0.tar.gz
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_ROOT}
                 -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE
                 -DCMAKE_PROGRAM_PATH:PATH=${EXTERNAL_ROOT}/bin
                 -DCMAKE_LIBRARY_PATH:PATH=${EXTERNAL_ROOT}/lib
                 -DCMAKE_INCLUDE_PATH:PATH=${EXTERNAL_ROOT}/include
                 -DDUNE_USE_ONLY_STATIC_LIBS=TRUE
                 -Ddune-common_DIR=${EXTERNAL_ROOT}/src/dune-common-build
                 -Ddune-geometry_DIR=${EXTERNAL_ROOT}/src/dune-geometry-build
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
  )

  ExternalProject_Add(
      dune-localfunctions
      DEPENDS dune-grid dune-geometry dune-common
      PREFIX ${EXTERNAL_ROOT}
      URL http://www.dune-project.org/download/2.3.0/dune-localfunctions-2.3.0.tar.gz
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_ROOT}
                 -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE
                 -DCMAKE_PROGRAM_PATH:PATH=${EXTERNAL_ROOT}/bin
                 -DCMAKE_LIBRARY_PATH:PATH=${EXTERNAL_ROOT}/lib
                 -DCMAKE_INCLUDE_PATH:PATH=${EXTERNAL_ROOT}/include
                 -DDUNE_USE_ONLY_STATIC_LIBS=TRUE
                 -Ddune-common_DIR=${EXTERNAL_ROOT}/src/dune-common-build
                 -Ddune-geometry_DIR=${EXTERNAL_ROOT}/src/dune-geometry-build
                 -Ddune-grid_DIR=${EXTERNAL_ROOT}/src/dune-grid-build
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
  )

  ExternalProject_Add(
      dune-foamgrid
      DEPENDS dune-grid dune-geometry dune-common dune-localfunctions
      PREFIX ${EXTERNAL_ROOT}
      URL ${PROJECT_SOURCE_DIR}/contrib/dune/dune-foamgrid
      PATCH_COMMAND 
         ${CMAKE_COMMAND} -E copy_if_different
                          ${CMAKE_SOURCE_DIR}/cmake/dune-foamgrid.install.cmake
                          ${EXTERNAL_ROOT}/src/dune-foamgrid/CMakeLists.txt
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_ROOT}
                 -DROOT=${PROJECT_SOURCE_DIR}
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
  )

  # This file helps to create a fake dune project
  # It answers the question posed by the duneproject script, including the subsidiary
  # "this directory already exits..."
  file(
    WRITE ${EXTERNAL_ROOT}/src/bempp.dune.input
    "dune-bempp
dune-common dune-geometry dune-grid dune-localfunctions
1
me@me
y
y
"
  )

  # Create fake dune project, with the sole goal of generating a config.h file!
  # First, generate a new project
  ExternalProject_Add(
    dune-bempp
    DOWNLOAD_COMMAND dune-common/bin/duneproject < bempp.dune.input
    DEPENDS dune-foamgrid dune-foamgrid dune-grid dune-geometry dune-common dune-localfunctions
    PREFIX ${EXTERNAL_ROOT}
    INSTALL_COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
             ${EXTERNAL_ROOT}/src/dune-bempp-build/config.h 
             ${EXTERNAL_ROOT}/include/dune_config.h
  )
  # Rerun cmake to capture new dune install
  add_recursive_cmake_step(dune-bempp dune_FOUND DEPENDEES build)
endif()