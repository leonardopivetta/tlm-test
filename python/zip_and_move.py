import os
import sys
import shutil
from zipfile import *
from tqdm import tqdm


def findAllFiles(path, extension):
    dirs = os.listdir(path)
    paths = []
    for dir in dirs:
        if (os.path.isdir(path + "/" + dir)):
            paths += findAllFiles(path + "/" + dir, extension)
        else:
            if (extension in dir):
                paths.append(path + "/" + dir)
    return paths



def compressFolders(paths, zipPath):
    print("COMPRESSING")

    zf = ZipFile(os.path.join(zipPath, "logs.zip"), "w", compression=ZIP_DEFLATED, compresslevel=5)

    for path in tqdm(paths):
        zf.write(path, path.replace(zipPath, ""))

    size = sum([zinfo.file_size for zinfo in zf.filelist])
    return float(size)


if __name__ == "__main__":
    if(len(sys.argv) != 2):
        print("HELP:\narguments accepted: \n all,\n csv,\n csv_json,\n csv_json_log,\n log,\n json,\n avi,\n log_avi,\n")
        exit(0)

    home = os.path.expanduser("~")
    base_path = home + "/logs"

    usb_mount = ""
    for i in range(10):
        path = "/media/usb" + str(i)
        if os.path.ismount(path):
            usb_mount = path
            break
    if(usb_mount == ""):
        print("No usb stick found")
        exit(1)

    if(os.path.isfile(os.path.join(base_path, "logs.zip"))):
        os.remove(os.path.join(base_path, "logs.zip"))

    files = []
    arg = sys.argv[1]
    if(arg == "all"):
        arg = "csv_json_log_avi"
    if("csv" in arg):
        files += findAllFiles(base_path, ".csv")
    if("log" in arg):
        files += findAllFiles(base_path, ".log")
    if("json" in arg):
        files += findAllFiles(base_path, ".json")
    if("avi" in arg):
        files += findAllFiles(base_path, ".avi")

    print("Found {} files.".format(len(files)))
    print("Start zipping")

    if(os.path.isfile(os.path.join(usb_mount, "logs.zip"))):
        os.remove(os.path.join(usb_mount, "logs.zip"))


    compressFolders(files, base_path)

    print("Movind to stick")

    shutil.copy(base_path + "/logs.zip", usb_mount + "/logs.zip")
    exit(0)
