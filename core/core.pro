#----------------------- THIS FILE SHOULD NOT BE MODIFIED ----------------------
#
# It is the one that is used to do "SUBDIRS += starlab-core". For this reason 
# the examples are not included here. To compile the core as a standalone demo 
# with examples, please see "starlab-core-examples.pro"
#-------------------------------------------------------------------------------
TEMPLATE = subdirs
CONFIG += ordered

#--- ROOT OF STARLAB DOCUMENTATION
OTHER_FILES += mainpage.h

#--- SPECIFIES CORE CONFIGURATION
system(qmake -set STARLAB       $$PWD/starlab.prf)
system(qmake -set OCTREE        $$PWD/external/octree/octree.prf)

#--- THREE CORE BUILD APP/LIBRARIES
SUBDIRS += starlib   #< SHARED LIBRARY
SUBDIRS += starlab   #< GUI APPLICATION
SUBDIRS += starterm  #< TERMINAL APPLICATION

#--- DEPENDENCY
starlab.depends = starlib
starterm.depends = starlib

#--- AND THE CORE PLUGINS TO COMPLEMENT
SUBDIRS += plugins/render_bbox   #< the default renderer, applies to any model
SUBDIRS += plugins/gui_filemenu  #< gui/logic of "menu=>file"
SUBDIRS += plugins/gui_filter    #< gui/logic of "menu=>filter"
SUBDIRS += plugins/gui_mode      #< gui/logic of "menu=>mode"
SUBDIRS += plugins/gui_render    #< gui/logic of "menu=>render"
SUBDIRS += plugins/gui_windows   #< gui/logic of "menu=>windows"
SUBDIRS += plugins/gui_view      #< gui/logic of "menu=>view"

#--- NOT READY
#SUBDIRS += plugins/gui_decorate #< gui/logic of "menu=>decorate"

