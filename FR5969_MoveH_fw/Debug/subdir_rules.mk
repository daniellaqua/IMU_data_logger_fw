################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1010/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/bin/cl430" -vmspx --abi=eabi --data_model=restricted --include_path="C:/ti/ccs1010/ccs/ccs_base/msp430/include" --include_path="C:/ti/ccs1010/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/include" --advice:power=all --advice:hw_config=all -g --define=__MSP430FR5969__ --display_error_number --diag_warning=225 --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


