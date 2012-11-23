#PROJECT(telem_server_standalone)
##policy about handling empty elements in lists
cmake_policy(SET CMP0007 NEW)


SET (target "xcaldaq_client")

MESSAGE(STATUS "Entering build for ${target}")

###Generate a verbose makefile
SET (CMAKE_VERBOSE_MAKEFILE ON)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)



##### add the slocal tree
#INCLUDE_DIRECTORIES(${SLOCAL}/include)
#LINK_DIRECTORIES(${SLOCAL}/lib)

#libs=(gsd_serial2; dfb3)



# compiler flags
ADD_DEFINITIONS(-Wall)
ADD_DEFINITIONS(-O)
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64) 
ADD_DEFINITIONS(-DNDEBUG) 
#ADD_DEFINITIONS(-g2) 
 



#  LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/quickusb/libquickusbv2.0.0/)


# project source dir' in cmake would be ${CMAKE_SOURCE_DIR} 
#  'build dir' in cmake would be ${CMAKE_BINARY_DIR} 


FILE(GLOB foo_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc *.cpp *.c *.cxx *.C)

#LIST(REMOVE_ITEM foo_sources "main_unit_test_cdm.cc")
#IST(REMOVE_ITEM foo_sources "main_unit_test_matrix.cc")



#### need to remove any moc files and uic files from foo sources

# Current Project Include
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

##### these groupings are for usable folders inside msvc
# MESSAGE(STATUS "Sources: ${foo_sources}")
file(GLOB_RECURSE H_FILES_FOR_MSVC "*.h")

ADD_EXECUTABLE(${target} ${foo_sources} ${H_FILES_FOR_MSVC})
#ADD_EXECUTABLE(${target} ${foo_sources} ${foo_moc_sources} ${foo_UIS_H})
#TARGET_LINK_LIBRARIES(${target} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} )
#



# ##################################################################
# #
# #   xcaldaq support
# #
# #
# ###################################################################


# SET (XCALDAQ_LIB  ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/xcaldaq/src)
# INCLUDE_DIRECTORIES(${XCALDAQ_LIB})
# LINK_DIRECTORIES(${XCALDAQ_LIB})
# TARGET_LINK_LIBRARIES(${target} xcaldaq)

# ##################################################################
# #
# #   tnt support
# #
# #
# ###################################################################


# SET (TNT_LIB  ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tnt/src)
# INCLUDE_DIRECTORIES(${TNT_LIB})
# LINK_DIRECTORIES(${TNT_LIB})
# #TARGET_LINK_LIBRARIES(${target} tnt)


##################################################################
#
#   misc system libraries
#
###################################################################


TARGET_LINK_LIBRARIES(${target} pthread)
TARGET_LINK_LIBRARIES(${target} fltk)



##################################################################
#
#   install rule
#
###################################################################


#### import the enviroment variable SLOCAL
SET (SLOCAL $ENV{SLOCAL})

INSTALL(TARGETS ${target} 
  RUNTIME DESTINATION ${SLOCAL}/bin 
  )



