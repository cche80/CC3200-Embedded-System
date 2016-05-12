################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi -me -Ooff --include_path="/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/include" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/driverlib" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/inc" -g --gcc --define=ccs --define=cc3200 --diag_wrap=off --display_error_number --diag_warning=225 --printf_support=full --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

pin_mux_config.obj: ../pin_mux_config.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi -me -Ooff --include_path="/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/include" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/driverlib" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/inc" -g --gcc --define=ccs --define=cc3200 --diag_wrap=off --display_error_number --diag_warning=225 --printf_support=full --preproc_with_compile --preproc_dependency="pin_mux_config.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

startup_ccs.obj: /home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common/startup_ccs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi -me -Ooff --include_path="/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/include" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/driverlib" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/inc" -g --gcc --define=ccs --define=cc3200 --diag_wrap=off --display_error_number --diag_warning=225 --printf_support=full --preproc_with_compile --preproc_dependency="startup_ccs.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

uart_if.obj: /home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common/uart_if.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi -me -Ooff --include_path="/home/chen/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.7/include" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/example/common" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/driverlib" --include_path="/home/chen/ti/CC3200SDK_1.2.0/cc3200-sdk/inc" -g --gcc --define=ccs --define=cc3200 --diag_wrap=off --display_error_number --diag_warning=225 --printf_support=full --preproc_with_compile --preproc_dependency="uart_if.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


