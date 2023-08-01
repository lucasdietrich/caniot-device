# platformio extra script

Import("env")

# import pprint
# d = env.Dictionary()
# pprint.pprint(d)
# pprint.pprint(build_flags)
# pprint.pprint(env)

build_flags = env.ParseFlags(env.GetProjectOption("build_flags"))
defines = {define: value for (define, value) in build_flags.get("CPPDEFINES")}

infos = dict()
infos['env_name'] = env['PIOENV']

infos['firmware_version'] = int(defines.get("__FIRMWARE_VERSION__", ""), 16)
infos['device_name'] = defines.get("__DEVICE_NAME__", "").strip('"')
infos['device_sid'] = int(defines.get("__DEVICE_SID__", ""), 16)
infos['device_cls'] = int(defines.get("__DEVICE_CLS__", ""), 16)
infos['device_did'] = (infos['device_cls']) | (infos['device_sid'] << 3)
infos['device_magic'] = int(defines.get("__MAGIC_NUMBER__", ""), 16)

infos['linker_script'] = build_flags.get("LINKFLAGS")[0]
infos['application_version'] = env.GetProjectOption("custom_application_version", "0.0.0")

infos['platform'] = env.GetProjectOption("platform")
infos['board'] = env.GetProjectOption("board")
infos['framework'] = "".join(env.GetProjectOption("framework"))

infos['defines_str'] = "\n    ".join(sorted(f"{k}={v}" for k, v in defines.items()))

env.Replace(PROGNAME=f"{infos['env_name']}")

content = """ENV
    name: {env_name}

CANIOT
    name: {device_name}
    did: {device_cls},{device_sid} -> {device_did}
    magic: 0x{device_magic:08X}

APPLICATION:
    firmware version: 0x{firmware_version:04X}
    application version: {application_version}

TARGET
    platform: {platform}
    framework: {framework}
    board: {board}
    linker script: {linker_script}

BUILD FLAGS - DEFINES
    {defines_str}

FEATURES (todo)

LIBRARIES (todo)
"""

# create a file named target-infos.txt in the build directory
# which contains the device name, sid, cls and magic number
with open(env.subst("$BUILD_DIR/target-infos.txt"), "w") as f:
    f.write(content.format(**infos))