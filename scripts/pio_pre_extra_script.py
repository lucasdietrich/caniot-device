# platformio extra script

import subprocess

Import("env")

# return latest commit hash
def get_commit_hash():
    ret = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    ret = ret.stdout.decode().strip()
    return ret

def get_is_dirty():
    ret = subprocess.run(["git", "diff", "--quiet"], capture_output=True)
    return int(ret.returncode != 0)

def build_commit_hash_defs():
    defs = []
    commit = get_commit_hash()
    length = len(commit)
    for i in range(0, length // 2):
        defs.append(f"-D__BUILD_COMMIT_B{i}__=0x" + commit[2*i:2*i+2])
    return defs

# return UTC timestamp in seconds
def get_build_date() -> int:
    ret = subprocess.run(["date", "+%s"], capture_output=True)
    return f"0x{int(ret.stdout.decode().strip()):08x}"

build_date = get_build_date()

env.Append(
    BUILD_FLAGS=[
        "-D__BUILD_DATE__=" + build_date,
        "-D__BUILD_DIRTY__=" + str(get_is_dirty()),
    ] + build_commit_hash_defs()
)

build_flags = env.ParseFlags(env.GetProjectOption("build_flags"))
defines = {define: value for (define, value) in build_flags.get("CPPDEFINES")}

infos = dict()
infos['env_name'] = env['PIOENV']
infos['commit'] = get_commit_hash()
infos['build_date'] = build_date

infos['firmware_version'] = int(defines.get("__FIRMWARE_VERSION__", ""), 16)
infos['device_name'] = defines.get("__DEVICE_NAME__", "").strip('"')
infos['device_sid'] = int(defines.get("__DEVICE_SID__", ""), 16)
infos['device_cls'] = int(defines.get("__DEVICE_CLS__", ""), 16)
infos['device_did'] = (infos['device_cls']) | (infos['device_sid'] << 3)
infos['device_magic'] = int(defines.get("__MAGIC_NUMBER__", ""), 16)

infos['linker_script'] = build_flags.get("LINKFLAGS")[0]

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
    commit: {commit}
    build date: {build_date}
    firmware version: 0x{firmware_version:04X}

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