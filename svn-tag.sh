#!/bin/sh
 
REPO_ROOT=`svn info | grep URL | sed 's/URL: //' | sed 's/\s+//g' | sed 's/trunk//'`

#REPO_ROOT=svn+ssh://xcalserver/svnrepos/labcal/calorimeter/xcaldaq_client/
echo "\n Looking in" 
echo ${REPO_ROOT}/tags
echo "______________________________________________"

#svn list ${REPO_ROOT}/tags

svn copy ${REPO_ROOT}/trunk ${REPO_ROOT}/tags/${1}
