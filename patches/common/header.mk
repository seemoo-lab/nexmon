header: FORCE
	@printf "\n\n"
	@printf "          ###########   ########### \033[0;33m  ##########    ##########  \033[0m         \n"
	@printf "         ############  ############ \033[0;33m ############  ############ \033[0m         \n"
	@printf "         ##            ##           \033[0;33m ##   ##   ##  ##        ## \033[0m         \n"
	@printf "         ##            ##           \033[0;33m ##   ##   ##  ##        ## \033[0m         \n"
	@printf "         ###########   ####  ###### \033[0;33m ##   ##   ##  ##    ###### \033[0m         \n"
	@printf "          ###########  ####  #      \033[0;33m ##   ##   ##  ##    #    # \033[0m         \n"
	@printf "                   ##  ##    ###### \033[0;33m ##   ##   ##  ##    #    # \033[0m         \n"
	@printf "                   ##  ##    #      \033[0;33m ##   ##   ##  ##    #    # \033[0m         \n"
	@printf "         ############  ##### ###### \033[0;33m ##   ##   ##  ##### ###### \033[0m         \n"
	@printf "         ###########    ########### \033[0;33m ##   ##   ##   ##########  \033[0m         \n"
	@printf "\n"
	@printf "            S E C U R E   M O B I L E   N E T W O R K I N G               \n"
	@printf "\n\n"
	@printf "                               presents:                                  \n"
	@printf "\n"
	@printf "              # ###   ###  \033[0;34m#  \033[0;31m #\033[0m # ###  ###   ###  # ###                  \n"
	@printf "              ##   # #   # \033[0;34m # \033[0;31m# \033[0m ##   ##   # #   # ##   #                 \n"
	@printf "              #    # ##### \033[0;32m  #  \033[0m #    #    # #   # #    #                 \n"
	@printf "              #    # #     \033[0;33m # \033[0;32m# \033[0m #    #    # #   # #    #                 \n"
	@printf "              #    #  #### \033[0;33m#  \033[0;32m #\033[0m #    #    #  ###  #    #                 \n"
	@printf "\n"
	@printf "                The C-based Firmware Patching Framework                   \n"
	@printf "\n\n"
	@printf "                           \033[0;31m!!! WARNING !!!\033[0m                                \n"
	@printf "    Our software may damage your hardware and may void your hardware’s    \n"
	@printf "     warranty! You use our tools at your own risk and responsibility      \n"
	@printf "\n\n"
ifeq ("$(wildcard $(NEXMON_ROOT)/DISABLE_STATISTICS)","")
	@printf "\033[0;31m  COLLECTING STATISTICS\033[0m read $(NEXMON_ROOT)/STATISTICS.md for more information\n"
	@make -s -f $(NEXMON_ROOT)/patches/common/statistics.mk
else
	@printf "\033[0;31m  STATISTICS DISABLED\033[0m to enable: delete $(NEXMON_ROOT)/DISABLE_STATISTICS\n"
endif



FORCE:
	
