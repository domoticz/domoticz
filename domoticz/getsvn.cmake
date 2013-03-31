# the FindSubversion.cmake module is part of the standard distribution
include(FindSubversion)
# extract working copy information for SOURCE_DIR into MY_XXX variables
Subversion_WC_INFO(${SOURCE_DIR} MY)
# write a file with the SVNVERSION define
file(WRITE svnversion.h.txt "#define SVNVERSION ${MY_WC_REVISION}\n")
# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        svnversion.h.txt svnversion.h)
