from fabric2 import Connection
from colorama import Fore, Back, Style

HOSTNAME = "192.168.10.154"
USER = "pi"
PASSWORD = "pip23"

project_name = "AlarmController" # AlarmController, ProdAlarmController
hex_name = f"{project_name}.hex"
elf_name = f"{project_name}.elf"

files = {
	"hex":
	{
		"local": f"./.pio/build/{project_name}/{hex_name}",
		"remote": f"binaries/{project_name}/{hex_name}"
	},
	"elf":
	{
		"local": f"./.pio/build/{project_name}/{elf_name}",
		"remote": f"binaries/{project_name}/{elf_name}"
	},
}

board = "ATMEGA328P"
port = "/dev/ttyACM0"

# connection
conn = Connection(f"{USER}@{HOSTNAME}", connect_kwargs={
	"password": PASSWORD
})

res = conn.run('uname -s', hide=False)

# create directory
conn.run(f"mkdir -p ~/binaries/{project_name}")

# copy files
for file in files.values():
	conn.put(file["local"], file["remote"])


# flash
flash_command = f"avrdude -c arduino -p {board} -P {port} -U flash:w:{files['hex']['remote']}"
result = conn.sudo(flash_command, hide=True, warn=True)

if result.return_code != 0:
	print(Fore.RED + f"Failed to flash {files['hex']['remote']} into device {port}\n\t" + flash_command + Style.RESET_ALL)
	print(result)
else:
	print(Fore.GREEN + "Successfully flashed device" + Style.RESET_ALL)
