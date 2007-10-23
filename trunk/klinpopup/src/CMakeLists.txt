
add_subdirectory(icons)

include_directories(${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(klinpopup_SRCS 
   main.cpp 
   klinpopup.cpp 
   klinpopupview.cpp 
   systemtray.cpp 
   makepopup.cpp 
   prefs.cpp)

kde4_add_ui_files(klinpopup_SRCS klinpopupview_base.ui prefs_base.ui)

kde4_add_kcfg_files(klinpopup_SRCS settings.kcfgc)

kde4_add_executable(klinpopup ${klinpopup_SRCS})

target_link_libraries(klinpopup ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS})

install(TARGETS klinpopup DESTINATION ${BIN_INSTALL_DIR})

install(PROGRAMS receivepopup.sh DESTINATION ${BIN_INSTALL_DIR} )
install(FILES klinpopup.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES klinpopup.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install(FILES klinpopupui.rc DESTINATION ${DATA_INSTALL_DIR}/klinpopup )
install(FILES ar_on.png ar_off.png DESTINATION ${DATA_INSTALL_DIR}/klinpopup )
