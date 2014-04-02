# Pulls together different dune components, some which are exported, some which are not.

# Look for common library first
if(Dune_REQUIRED)
    list(APPEND arguments REQUIRED)
endif()
unset(quietly)
if(Dune_FIND_QUIETLY)
    list(APPEND arguments QUIET)
    set(quietly QUIET)
endif()
find_package(dune-common ${arguments})
if(NOT dune-common_FOUND)
    return()
endif()

function(is_exported_component component OUTVAR)
    list(FIND "foamgrid" ${component} found)
    if(found GREATER -1)
        set(${OUTVAR} FALSE PARENT_SCOPE)
    else()
        set(${OUTVAR} TRUE PARENT_SCOPE)
    endif()
endfunction()

get_filename_component(Dune_INCLUDE_COMPONENT_DIR
    ${dune-common_INCLUDE_DIRS} PATH)
unset(required_not_found)
foreach(component ${cmaked_components})
    is_exported_component(${component} is_exported)
    if(is_exported)
        unset(required)
        if(Dune_FIND_REQUIRED_${component})
            set(required REQUIRED)
        endif()
        find_package(dune-${component} ${quietly} ${required})
    elseif(IS_DIRECTORY ${Dune_INCLUDE_COMPONENT_DIR}/${component})
        set(dune-${component}_FOUND TRUE)
    elseif(Dune_FIND_REQUIRED-${component})
        list(APPEND required_not_found ${component})
    endif()
endforeach()
if("${required_not_found}" STREQUAL "")
    set(Dune_FOUND TRUE)
else()
    message(FATAL_ERROR "Could not find dune components ${required_not_found}")
endif()