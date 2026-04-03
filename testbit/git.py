# See <https://git-scm.com/docs/git-check-ref-format>
def check_ref_format(branch: str) -> int:
    if not branch:
        return -1

    slashseparated = branch.split("/")
    for x in slashseparated:
        # 6
        if not x:
            return 6
        # 1
        if x.startswith(".") or x.endswith(".lock"):
            return 1

    # 3
    if ".." in branch:
        return 3
    # 4, Not entierly correct, but this is better
    if any(ord(c) < 0o40 or ord(c) >= 177 or c in "~^:" for c in branch):
        return 4
    # 5
    if any(c in "?*[" for c in branch):
        return 5
    # 7
    if branch.endswith("."):
        return 7
    # 8
    if "@{" in branch:
        return 8
    # 9
    if branch == "@":
        return 9
    # 10
    if "\\" in branch:
        return 10

    return 0
