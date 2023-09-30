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

env.Append(
    BUILD_FLAGS=[
        "-D__BUILD_DATE__=" + get_build_date(),
        "-D__BUILD_DIRTY__=" + str(get_is_dirty()),
    ] + build_commit_hash_defs()
)