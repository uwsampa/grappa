macro(grappa_search_for_boost)
  #
  # first, see if we already built a copy of boost in third-party; prefer that over any other boost
  #

  # save user-specified pointers to boost
  if(DEFINED BOOSTROOT)
    set(BOOSTROOT_BACKUP ${BOOST_ROOT})
    unset(BOOSTROOT)
  endif()
  if(DEFINED BOOST_ROOT)
    set(BOOST_ROOT_BACKUP ${BOOST_ROOT})
  endif()

  if(DEFINED Boost_NO_SYSTEM_PATHS)
  set(Boost_NO_SYSTEM_PATHS_BACKUP ${Boost_NO_SYSTEM_PATHS})
  set(Boost_NO_SYSTEM_PATHS_BACKUP)
  endif()

  # now look in third-party
  set(BOOST_ROOT ${THIRD_PARTY_ROOT})
  set(Boost_NO_SYSTEM_PATHS 1)
  if(VERBOSE)
    message("---------- Searching in third-party with static=${Boost_USE_STATIC_LIBS}....")
    find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND})
  else()
    find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND} QUIET)
  endif()

  #
  # if boost was not found in third-party, restore any user-specified pointers and search there and in standard paths
  #
  if(NOT Boost_FOUND)
    
    # restore any user-specified pointers
    if(Boost_NO_SYSTEM_PATHS_BACKUP)
      set(Boost_NO_SYSTEM_PATHS ${Boost_NO_SYSTEM_PATHS_BACKUP})
    else()
      unset(Boost_NO_SYSTEM_PATHS)
    endif()
    if(BOOSTROOT_BACKUP)
      set(BOOSTROOT ${BOOSTROOT_BACKUP})
    endif()
    if(BOOST_ROOT_BACKUP)
      set(BOOST_ROOT ${BOOST_ROOT_BACKUP})
    endif()
    
    # search for single-threaded version
    if(VERBOSE)
      message("---------- Searching for single-threaded with static=${Boost_USE_STATIC_LIBS}....")
      find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND})
    else()
      find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND} QUIET)
    endif()
    
    # if no single-threaded, try multi-threaded next
    if(NOT Boost_FOUND)
      set(Boost_USE_MULTITHREADED ON)
      if(VERBOSE)
        message("---------- Searching for multi-threaded with static=${Boost_USE_STATIC_LIBS}....")
        find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND})
      else()
        find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS_TO_FIND} QUIET)
      endif()
    endif()
  endif()
endmacro()

