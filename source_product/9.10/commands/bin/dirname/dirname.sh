
# @(#) $Revision: 38.1 $    
expr \
  ${1-.}'/' : '\(/\)[^/]*/$' \
  \| ${1-.}'/' : '\(.*[^/]\)//*[^/][^/]*//*$' \
  \| .
