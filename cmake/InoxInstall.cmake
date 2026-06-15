# SPDX-License-Identifier: MPL-2.0
# Minimal install rule; release packaging scripts may layer on top of this.

include(GNUInstallDirs)
install(TARGETS inox RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
