qemu:
	qemu-system-avr -M uno -bios .pio/build/HeatingController/firmware.elf -nographic -s -S


gdb:
	avr-gdb -x gdbinit .pio/build/HeatingController/firmware.elf

dis_explicit:
	rm -rf tmp/HeatingController
	python3 ./lib/AVRTOS/scripts/pydis.py
	rm -rf tmp/EXPLICIT_MAIN_STACK
	mv tmp/HeatingController tmp/EXPLICIT_MAIN_STACK

dis_implicit:
	rm -rf tmp/HeatingController
	python3 ./lib/AVRTOS/scripts/pydis.py
	rm -rf tmp/IMPLICIT_MAIN_STACK
	mv tmp/HeatingController tmp/IMPLICIT_MAIN_STACK

dis:
	python3 ./lib/AVRTOS/scripts/pydis.py