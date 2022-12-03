qemu:
	qemu-system-avr -M uno -bios .pio/build/HeatingController/firmware.elf -nographic -s -S

dis:
	python3 ./lib/AVRTOS/scripts/pydis.py